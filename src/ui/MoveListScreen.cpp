#include "ui/MoveListScreen.h"
#include "core/InputMap.h"
#include "game/CharacterVisual.h"
#include "game/Player.h"
#include "game/moves/MoveRegistry.h"
#include "ui/TouchControls.h"
#include "raylib.h"
#include <algorithm>
#include <cmath>
#include <cstring>

// Este archivo dibuja con ::DrawText de raylib (sin la traduccion automatica de
// SpanishText.h: los nombres de movimiento ya vienen en espanol y la tabla de
// traduccion reemplaza fragmentos como "SP" o "HP" dentro de cualquier palabra).

namespace district_fury {
namespace ui {

// ---------------------------------------------------------------------------
// Medidas
// ---------------------------------------------------------------------------

int MeasureUiText(const char* text, int fontSize) {
    if (!text) return 0;
    if (IsWindowReady()) return MeasureText(text, fontSize);
    // Sin ventana (tests): la fuente por defecto mide como mucho ~0.8 * size por
    // letra (M, W); se usa esa cota para que la prueba sea conservadora.
    return static_cast<int>(std::strlen(text)) * fontSize * 8 / 10;
}

int FitFontSize(const char* text, int maxWidth, int fontSize) {
    int fs = fontSize;
    while (fs > kMoveMinFont && MeasureUiText(text, fs) > maxWidth) --fs;
    return fs;
}

// ---------------------------------------------------------------------------
// Construccion de la lista
// ---------------------------------------------------------------------------

namespace {

MoveRow MakeRow(Player& p, AttackId id, const char* input, const char* note = "") {
    const AttackDef d = p.EffectiveDef(id);
    MoveRow r;
    r.id = id;
    r.name = (d.name && d.name[0]) ? d.name : AttackIdName(id);
    r.input = input;
    r.note = note;
    r.damage = d.damage;
    r.hits = std::max(1, d.hitCount);
    r.spCost = d.spCost;
    return r;
}

MoveRow PlainRow(const char* name, const char* input, const char* note) {
    MoveRow r;
    r.name = name;
    r.input = input;
    r.note = note;
    return r;
}

void AddIf(Player& p, MovePage& page, AttackId id, const char* input, const char* note = "") {
    if (p.MoveEnabled(id)) page.rows.push_back(MakeRow(p, id, input, note));
}

}  // namespace

std::vector<MovePage> BuildMoveList(Player& p) {
    std::vector<MovePage> out;
    const MoveSet& ms = p.Moves();
    const bool legacy = p.IsKfCharacter() && std::strcmp(ms.name, "legado-clip") == 0;
    const unsigned caps = ms.caps;

    // ---- Cadenas ----
    MovePage chains{"CADENAS", {}};
    auto addChain = [&](const int* links, int count, char letter) {
        std::string input;
        for (int i = 0; i < count; ++i) {
            const int raw = links[i];
            if (raw <= 0 || raw >= kAttackIdCount) break;
            const AttackId id = static_cast<AttackId>(raw);
            if (!p.MoveEnabled(id)) break;   // un eslabon inutilizable corta la cadena
            if (i > 0) input += ' ';
            input += letter;
            chains.rows.push_back(MakeRow(p, id, input.c_str()));
        }
    };
    addChain(ms.chainJ, std::min(ms.chainJCount, 6), 'J');
    addChain(ms.chainK, std::min(ms.chainKCount, 4), 'K');
    if (!legacy) AddIf(p, chains, AttackId::KickLauncher, "J J K", "lanza al rival");
    if (!chains.rows.empty()) out.push_back(chains);

    // ---- Direccionales ----
    MovePage dir{"DIRECCIONALES", {}};
    if (!legacy) {
        AddIf(p, dir, AttackId::FwdPunch, ">+J");
        AddIf(p, dir, AttackId::FwdKick, ">+K");
        AddIf(p, dir, AttackId::BackElbow, "<+J", "sin girar");
        AddIf(p, dir, AttackId::SpinKick, "<+K", "golpea a los dos lados");
        AddIf(p, dir, AttackId::Uppercut, "v+J", "lanza al rival");
        AddIf(p, dir, AttackId::Stomp, "v+J", "sobre rival caido");
        AddIf(p, dir, AttackId::Sweep, "v+K", "bajo");
        AddIf(p, dir, AttackId::DashAttack, "SHIFT+J", "en carrera");
        AddIf(p, dir, AttackId::DashKick, "SHIFT+K", "deslizamiento");
        if (caps & CapBackDash) dir.rows.push_back(PlainRow("RETRODASH", "<+SHIFT", "esquiva hacia atras"));
    }
    if (!dir.rows.empty()) out.push_back(dir);

    // ---- Aire ----
    MovePage air{"AIRE", {}};
    if (!legacy && (caps & CapJump)) {
        air.rows.push_back(PlainRow("SALTO", "I / SALTO", "tecla I o boton"));
        AddIf(p, air, AttackId::AirPunch, "J", "en el aire");
        AddIf(p, air, AttackId::AirPunch2, "J J", "en el aire");
        AddIf(p, air, AttackId::AirKick, "K", "en el aire");
        AddIf(p, air, AttackId::DiveKick, "v+K", "patada picada");
        AddIf(p, air, AttackId::AirSpike, ">+J", "martillo");
    }
    if (!air.rows.empty()) out.push_back(air);

    // ---- Especiales / Signature ----
    MovePage spec{"ESPECIALES", {}};
    AddIf(p, spec, AttackId::EnergyWave, "QCF+J / L");
    if (!legacy) {
        AddIf(p, spec, AttackId::RisingUppercut, "DP+K", "invulnerable al subir");
        AddIf(p, spec, AttackId::Hurricane, "QCB+K");
        AddIf(p, spec, AttackId::DrillKick, "QCF+K");
        AddIf(p, spec, AttackId::Signature1, "QCB+J");
        AddIf(p, spec, AttackId::Signature2, ">+L");
        AddIf(p, spec, AttackId::Signature3, "v+L");
    }
    if (!spec.rows.empty()) out.push_back(spec);

    // ---- Furia ----
    MovePage rage{"FURIA", {}};
    rage.rows.push_back(PlainRow("ENTRAR EN FURIA", "ESPACIO", "con la barra llena"));
    AddIf(p, rage, AttackId::RageAttack, "ESPACIO", "ya en furia");
    AddIf(p, rage, AttackId::Finisher, "K", "furia, combo de 4+");
    out.push_back(rage);

    // ---- Defensa / Agarre ----
    MovePage def{"DEFENSA Y AGARRE", {}};
    def.rows.push_back(PlainRow("BLOQUEO", "B", "reduce el dano"));
    def.rows.push_back(PlainRow("DASH", "SHIFT", "invulnerable un instante"));
    AddIf(p, def, AttackId::GuardCounter, "J+K", "durante el bloqueo");
    AddIf(p, def, AttackId::Grab, "J+K", "cerca del rival");
    AddIf(p, def, AttackId::GrabKnee, "J", "agarrando");
    AddIf(p, def, AttackId::ThrowForward, ">+K", "agarrando");
    AddIf(p, def, AttackId::ThrowBack, "<+K", "agarrando");
    AddIf(p, def, AttackId::WakeupAttack, "J", "al levantarte");
    out.push_back(def);

    // ---- Habilidades 1-5 (y 6 = transformar) ----
    MovePage skills{"HABILIDADES", {}};
    if (p.IsKfCharacter()) {
        // Habilidades de clip: se reparten de 5 en 5 y TAB cambia de pagina.
        const int savedPage = p.skillPage;
        const int pages = std::max(1, p.SkillPageCount());
        for (int pg = 0; pg < pages; ++pg) {
            p.skillPage = pg;
            for (int i = 0; i < Player::kSkillCount - 1; ++i) {
                const char* nm = p.SkillName(i);
                if (!nm || !nm[0] || std::strcmp(nm, "-") == 0) continue;
                std::string input;
                for (int t = 0; t < pg; ++t) input += "TAB ";
                input += static_cast<char>('1' + i);
                MoveRow r;
                r.name = nm;
                r.input = input;
                r.note = pg > 0 ? "pagina " + std::to_string(pg + 1) : "espera 15 s";
                skills.rows.push_back(r);
            }
        }
        p.skillPage = savedPage;
    } else {
        for (int i = 0; i < Player::kSkillCount - 1; ++i) {
            // Mismo orden de busqueda que Player::TryStartSkill.
            AttackId id = static_cast<AttackId>(ms.skillSlots[i]);
            if (!p.MoveEnabled(id)) id = static_cast<AttackId>(DefaultSkillSlot(i));
            if (!p.MoveEnabled(id)) id = AttackId::Punch3;
            const char num[2] = {static_cast<char>('1' + i), 0};
            skills.rows.push_back(MakeRow(p, id, num, "espera 15 s"));
        }
    }
    {
        MoveRow r = PlainRow("TRANSFORMAR", "6", "12 s: +dano +velocidad");
        skills.rows.push_back(r);
    }
    out.push_back(skills);
    return out;
}

// ---------------------------------------------------------------------------
// Entradas con iconos
// ---------------------------------------------------------------------------

namespace {

constexpr int kIconW = 20, kIconH = 22, kKeyFont = 13, kAtomGap = 3, kPlusW = 11, kSeqGap = 8, kSlashW = 18;

struct Atom {
    enum Kind { Arrow, Key, Plus, Slash, Gap } kind = Key;
    float angle = 0;   // grados, 0 = derecha, 90 = abajo
    std::string text;
    int w = 0;
};

void PushKey(std::vector<Atom>& v, const std::string& t) {
    Atom a;
    a.kind = Atom::Key;
    a.text = t;
    a.w = std::max(kIconH, MeasureUiText(t.c_str(), kKeyFont) + 10);
    v.push_back(a);
}

void PushArrow(std::vector<Atom>& v, float angle) {
    Atom a;
    a.kind = Atom::Arrow;
    a.angle = angle;
    a.w = kIconW;
    v.push_back(a);
}

void PushAtom(std::vector<Atom>& v, const std::string& s) {
    if (s == ">") PushArrow(v, 0);
    else if (s == "v") PushArrow(v, 90);
    else if (s == "<") PushArrow(v, 180);
    else if (s == "^") PushArrow(v, -90);
    else if (s == "QCF") { PushArrow(v, 90); PushArrow(v, 45); PushArrow(v, 0); }
    else if (s == "QCB") { PushArrow(v, 90); PushArrow(v, 135); PushArrow(v, 180); }
    else if (s == "DP") { PushArrow(v, 0); PushArrow(v, 90); PushArrow(v, 45); }
    else PushKey(v, s);
}

std::vector<Atom> Layout(const std::string& input) {
    std::vector<Atom> v;
    size_t i = 0;
    bool firstToken = true;
    while (i < input.size()) {
        size_t j = input.find(' ', i);
        if (j == std::string::npos) j = input.size();
        const std::string tok = input.substr(i, j - i);
        i = j + 1;
        if (tok.empty()) continue;
        if (!firstToken) v.push_back(Atom{Atom::Gap, 0, "", kSeqGap});
        firstToken = false;
        if (tok == "/") { v.push_back(Atom{Atom::Slash, 0, "/", kSlashW}); continue; }
        size_t a = 0;
        bool firstAtom = true;
        while (a <= tok.size()) {
            size_t b = tok.find('+', a);
            if (b == std::string::npos) b = tok.size();
            const std::string part = tok.substr(a, b - a);
            a = b + 1;
            if (part.empty()) { if (b >= tok.size()) break; continue; }
            if (!firstAtom) v.push_back(Atom{Atom::Plus, 0, "+", kPlusW});
            firstAtom = false;
            PushAtom(v, part);
            if (b >= tok.size()) break;
        }
    }
    return v;
}

Color KeyColor(const std::string& t) {
    if (t == "J") return {200, 50, 44, 255};
    if (t == "K") return {206, 124, 36, 255};
    if (t == "L") return {48, 140, 220, 255};
    return {66, 76, 92, 255};
}

void DrawArrowIcon(int x, int y, float angleDeg) {
    DrawRectangle(x, y, kIconW, kIconH, {24, 34, 48, 255});
    DrawRectangleLines(x, y, kIconW, kIconH, {70, 110, 140, 255});
    const float cx = x + kIconW * 0.5f, cy = y + kIconH * 0.5f, r = 7.0f;
    const float rad = angleDeg * 3.14159265f / 180.0f;
    const float dx = std::cos(rad), dy = std::sin(rad);
    const Vector2 tip{cx + dx * r, cy + dy * r};
    const Vector2 base{cx - dx * r * 0.7f, cy - dy * r * 0.7f};
    const Vector2 p1{base.x - dy * r * 0.85f, base.y + dx * r * 0.85f};
    const Vector2 p2{base.x + dy * r * 0.85f, base.y - dx * r * 0.85f};
    DrawTriangle(tip, p1, p2, {120, 225, 255, 255});
    DrawTriangle(tip, p2, p1, {120, 225, 255, 255});
}

void DrawInputIcons(const std::string& input, int x, int y) {
    const std::vector<Atom> atoms = Layout(input);
    int cx = x;
    for (size_t i = 0; i < atoms.size(); ++i) {
        const Atom& a = atoms[i];
        switch (a.kind) {
            case Atom::Arrow: DrawArrowIcon(cx, y, a.angle); break;
            case Atom::Key: {
                const Color c = KeyColor(a.text);
                DrawRectangle(cx, y, a.w, kIconH, c);
                DrawRectangleLines(cx, y, a.w, kIconH, {235, 240, 245, 200});
                ::DrawText(a.text.c_str(), cx + (a.w - MeasureUiText(a.text.c_str(), kKeyFont)) / 2, y + (kIconH - kKeyFont) / 2,
                           kKeyFont, WHITE);
                break;
            }
            case Atom::Plus:
                DrawRectangle(cx + 2, y + kIconH / 2 - 1, kPlusW - 4, 2, {190, 200, 210, 255});
                DrawRectangle(cx + kPlusW / 2 - 1, y + kIconH / 2 - 4, 2, 8, {190, 200, 210, 255});
                break;
            case Atom::Slash:
                DrawLineEx({static_cast<float>(cx + 5), static_cast<float>(y + kIconH - 4)},
                           {static_cast<float>(cx + kSlashW - 5), static_cast<float>(y + 4)}, 2.0f, {160, 175, 190, 255});
                break;
            case Atom::Gap: break;
        }
        cx += a.w;
        if (a.kind != Atom::Gap && a.kind != Atom::Plus && i + 1 < atoms.size() && atoms[i + 1].kind != Atom::Plus &&
            atoms[i + 1].kind != Atom::Gap && a.kind != Atom::Slash && atoms[i + 1].kind != Atom::Slash)
            cx += kAtomGap;
    }
}

}  // namespace

int MoveInputWidth(const std::string& input) {
    const std::vector<Atom> atoms = Layout(input);
    int w = 0;
    for (size_t i = 0; i < atoms.size(); ++i) {
        w += atoms[i].w;
        if (atoms[i].kind != Atom::Gap && atoms[i].kind != Atom::Plus && i + 1 < atoms.size() &&
            atoms[i + 1].kind != Atom::Plus && atoms[i + 1].kind != Atom::Gap && atoms[i].kind != Atom::Slash &&
            atoms[i + 1].kind != Atom::Slash)
            w += kAtomGap;
    }
    return w;
}

// ---------------------------------------------------------------------------
// Pantalla
// ---------------------------------------------------------------------------

namespace {

constexpr Rectangle kPanel{50, 36, 1180, 648};
constexpr int kRowsTop = 168, kRowH = 36, kVisibleRows = 12;
constexpr int kNameX = 80, kInputX = 385, kDmgX = 825, kSpX = 910, kNoteX = 995;
constexpr Rectangle kCloseBtn{1090, 50, 124, 32};

Rectangle TabRect(int i, int n) {
    const float total = kPanel.width - 60.0f;
    const float w = std::min(190.0f, total / static_cast<float>(std::max(1, n)));
    return {kPanel.x + 30.0f + w * static_cast<float>(i), 96.0f, w - 6.0f, 32.0f};
}

bool Hit(Rectangle r, Vector2 p) { return CheckCollisionPointRec(p, r); }

}  // namespace

MoveListScreen& MoveList() {
    static MoveListScreen s;
    return s;
}

bool MoveListRequested(Rectangle tapArea, bool allowEnter) {
    if (input::Pressed(KEY_M) || (allowEnter && input::Pressed(KEY_ENTER))) return true;
    Vector2 tap;
    return touch::PeekTap(&tap) && CheckCollisionPointRec(tap, tapArea);
}

bool UpdateMoveListInPause(bool paused) {
    MoveListScreen& m = MoveList();
    if (!m.IsOpen()) return false;
    if (!paused) { m.Close(); return false; }
    m.Update();
    return true;
}

void MoveListScreen::Open(Player& player) {
    pages_ = BuildMoveList(player);
    title_ = std::string("MOVIMIENTOS  -  ") + ShortCharacterName(player.skin);
    open_ = true;
    page_ = cursor_ = scroll_ = 0;
}

void MoveListScreen::SelectPage(int p) {
    if (pages_.empty()) return;
    const int n = static_cast<int>(pages_.size());
    page_ = ((p % n) + n) % n;
    cursor_ = scroll_ = 0;
}

void MoveListScreen::Update() {
    if (!open_) return;
    if (input::Pressed(KEY_ESCAPE) || input::Pressed(KEY_M) || input::Pressed(KEY_ENTER) || input::Pressed(KEY_BACKSPACE)) {
        open_ = false;
        return;
    }
    if (pages_.empty()) return;
    const int n = static_cast<int>(pages_.size());
    if (input::Pressed(KEY_RIGHT) || input::Pressed(KEY_D) || input::Pressed(KEY_TAB)) SelectPage(page_ + 1);
    if (input::Pressed(KEY_LEFT) || input::Pressed(KEY_A)) SelectPage(page_ - 1);
    const int rows = static_cast<int>(pages_[static_cast<size_t>(page_)].rows.size());
    if (input::Pressed(KEY_DOWN) || input::Pressed(KEY_S)) cursor_ = std::min(cursor_ + 1, std::max(0, rows - 1));
    if (input::Pressed(KEY_UP) || input::Pressed(KEY_W)) cursor_ = std::max(cursor_ - 1, 0);
    // Toque / clic: pestana, fila o cerrar (coordenadas virtuales).
    Vector2 tap;
    if (touch::TakeTap(&tap)) {
        if (Hit(kCloseBtn, tap)) { open_ = false; return; }
        for (int i = 0; i < n; ++i)
            if (Hit(TabRect(i, n), tap)) { SelectPage(i); return; }
        if (tap.y >= kRowsTop && tap.y < kRowsTop + kRowH * kVisibleRows && tap.x > kPanel.x && tap.x < kPanel.x + kPanel.width) {
            const int row = scroll_ + static_cast<int>((tap.y - kRowsTop) / kRowH);
            if (row >= 0 && row < rows) cursor_ = row;
        } else if (tap.y >= kRowsTop + kRowH * kVisibleRows) {
            cursor_ = std::min(cursor_ + kVisibleRows, std::max(0, rows - 1));   // zona inferior: pagina siguiente de filas
        }
    }
    if (cursor_ < scroll_) scroll_ = cursor_;
    if (cursor_ >= scroll_ + kVisibleRows) scroll_ = cursor_ - kVisibleRows + 1;
    scroll_ = std::clamp(scroll_, 0, std::max(0, rows - kVisibleRows));
}

void MoveListScreen::Draw() const {
    if (!open_) return;
    DrawRectangle(0, 0, 1280, 720, {0, 0, 0, 200});
    DrawRectangleRec(kPanel, {7, 12, 18, 248});
    DrawRectangleLinesEx(kPanel, 2, {70, 200, 235, 170});
    ::DrawText(title_.c_str(), 80, 52, 28, {220, 232, 242, 255});
    DrawRectangleRec(kCloseBtn, {40, 18, 22, 255});
    DrawRectangleLinesEx(kCloseBtn, 2, {220, 90, 90, 255});
    ::DrawText("CERRAR", static_cast<int>(kCloseBtn.x) + 30, static_cast<int>(kCloseBtn.y) + 8, 16, WHITE);

    const int n = static_cast<int>(pages_.size());
    if (pages_.empty()) {
        ::DrawText("Sin movimientos.", 80, 200, 22, WHITE);
        return;
    }
    for (int i = 0; i < n; ++i) {
        const Rectangle r = TabRect(i, n);
        const bool sel = i == page_;
        DrawRectangleRec(r, sel ? Color{20, 56, 86, 255} : Color{14, 20, 28, 255});
        DrawRectangleLinesEx(r, sel ? 2.0f : 1.0f, sel ? Color{110, 225, 255, 255} : Color{60, 80, 100, 255});
        const char* t = pages_[static_cast<size_t>(i)].title.c_str();
        const int fs = FitFontSize(t, static_cast<int>(r.width) - 10, 14);
        ::DrawText(t, static_cast<int>(r.x + (r.width - MeasureUiText(t, fs)) / 2), static_cast<int>(r.y + (r.height - fs) / 2), fs,
                   sel ? WHITE : Color{150, 175, 195, 255});
    }
    // Encabezado de columnas.
    const Color head{120, 160, 185, 255};
    ::DrawText("MOVIMIENTO", kNameX, 144, 13, head);
    ::DrawText("ENTRADA", kInputX, 144, 13, head);
    ::DrawText("DANO", kDmgX, 144, 13, head);
    ::DrawText("SP", kSpX, 144, 13, head);
    ::DrawText("NOTA", kNoteX, 144, 13, head);
    DrawRectangle(70, 162, 1140, 1, {60, 90, 110, 255});

    const MovePage& page = pages_[static_cast<size_t>(page_)];
    const int rows = static_cast<int>(page.rows.size());
    for (int k = 0; k < kVisibleRows; ++k) {
        const int idx = scroll_ + k;
        if (idx >= rows) break;
        const MoveRow& r = page.rows[static_cast<size_t>(idx)];
        const int y = kRowsTop + k * kRowH;
        const bool sel = idx == cursor_;
        DrawRectangle(70, y, 1140, kRowH - 2, sel ? Color{22, 52, 78, 255} : (k % 2 ? Color{11, 17, 24, 255} : Color{14, 21, 30, 255}));
        if (sel) DrawRectangle(70, y, 4, kRowH - 2, {110, 225, 255, 255});
        const int nfs = FitFontSize(r.name.c_str(), kMoveNameColW, kMoveNameFont);
        ::DrawText(r.name.c_str(), kNameX, y + (kRowH - 2 - nfs) / 2, nfs, sel ? WHITE : Color{215, 225, 235, 255});
        DrawInputIcons(r.input, kInputX, y + (kRowH - 2 - kIconH) / 2);
        if (r.damage > 0) {
            const char* d = r.hits > 1 ? TextFormat("%dx%d", r.damage, r.hits) : TextFormat("%d", r.damage);
            ::DrawText(d, kDmgX, y + 9, 16, {255, 225, 170, 255});
        } else {
            ::DrawText("-", kDmgX, y + 9, 16, {90, 105, 120, 255});
        }
        if (r.spCost > 0) ::DrawText(TextFormat("%d", r.spCost), kSpX, y + 9, 16, {120, 210, 255, 255});
        if (!r.note.empty()) {
            const int fs = FitFontSize(r.note.c_str(), kMoveNoteColW, 13);
            ::DrawText(r.note.c_str(), kNoteX, y + (kRowH - 2 - fs) / 2, fs, {150, 175, 190, 255});
        }
    }
    if (scroll_ > 0) ::DrawText("^ mas", 1130, 146, 13, {255, 215, 90, 255});
    if (scroll_ + kVisibleRows < rows) ::DrawText("v mas", 1130, kRowsTop + kRowH * kVisibleRows + 2, 13, {255, 215, 90, 255});
    ::DrawText(TextFormat("%d/%d", page_ + 1, n), 1040, 56, 18, {170, 200, 220, 255});
    ::DrawText("A/D o IZQ/DER: PAGINA     W/S o ARRIBA/ABAJO: MOVER     ESC / M: CERRAR", 80, 648, 15, {140, 170, 190, 255});
    if (touch::Enabled())
        ::DrawText("TACTIL: toca una pestana o una fila", 80, 666, 13, {120, 150, 170, 255});
}

}  // namespace ui
}  // namespace district_fury
