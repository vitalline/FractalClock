#define _CRT_SECURE_NO_WARNINGS
#include <SFML/Graphics.hpp>
#include <SFML/OpenGL.hpp>
#include <iostream>
#include <ctime>
#include "HSV.h"
#include "resource.h"

static const float PI = 3.14159265359f;
static const int max_iters = 16;
static const int window_w_init = 700;
static const int window_h_init = 700;
static const sf::Color clock_face_color(255, 255, 255, 192);
static const sf::Color bgnd_color(16, 16, 16);
enum TimeType {
  REG, REG_24, SEN_3, SEN_4, DEC, DOZ, HEX, CRT, TIME_NUM
};
static const float time_offset_modes[] = {
  5.0f, 60.0f, 300.0f, 3600.0f,
  1.0f, 0.2f, 0.05f, 0.01f
};
static const float hand_offset_modes[] = {
  PI / 12.0f, PI / 36.0f, PI / 60.0f, PI / 180.0f,
  PI / 48.0f, PI / 144.0f, PI / 240.0f, PI / 720.0f
};
static const char* show_hands_name[] = {
  "[1-3] Show/Hide clock hands",
  "[1-4] Show/Hide clock hands",
};
static const char* select_hands_name[] = {
  "[Shift+1-3] Select clock hand",
  "[Shift+1-4] Select clock hand",
};
static const char* deselect_hands_name[] = {
  "[Shift+1] Deselect clock hand",
  "[Shift+2] Deselect clock hand",
  "[Shift+3] Deselect clock hand",
  "[Shift+4] Deselect clock hand",
};
static const char* time_type_name[] = {
  "[M/N] Regular clock",
  "[M/N] 24-hour clock",
  "[M/N] Heximal clock",
  "[M/N] Heximal clock (4 hands)",
  "[M/N] Decimal clock",
  "[M/N] Dozenal clock",
  "[M/N] Hexadecimal clock",
  "[M/N] Creata Standard Time",
};
static const char* time_offset_name[] = {
  "[+/-] Offset time by 5 seconds",
  "[+/-] Offset time by 1 minute",
  "[+/-] Offset time by 5 minutes",
  "[+/-] Offset time by 1 hour",
  "[+/-] Offset time by 1 second",
  "[+/-] Offset time by 200 ms",
  "[+/-] Offset time by 50 ms",
  "[+/-] Offset time by 10 ms",
};
// DEG° MIN’ SEC”
static const char* hand_offset_name[] = {
  "[+/-] Offset hand by 15°",
  "[+/-] Offset hand by 5°",
  "[+/-] Offset hand by 3°",
  "[+/-] Offset hand by 1°",
  "[+/-] Offset hand by 3.75°",
  "[+/-] Offset hand by 1.25°",
  "[+/-] Offset hand by 0.75°",
  "[+/-] Offset hand by 0.25°",
};
static const char* change_offset_name = "[Shift/Ctrl/Alt] Offset step";
static const char* reset_time_offset_name[] = {
  "[0] Reset timer",
  "[0] Reset time offset",
};
static const char* reset_hand_offset_name = "[Shift+0] Reset hand offset";
static const char* pause_time_name[] = {
  "[P] Pause time",
  "[P] Resume time",
};
static const char* real_time_name[] = {
  "[R] Timer",
  "[R] Timer (paused)",
  "[R] Timer", //Intentionally not using "Timer (offset)" here because the time being offset only really matters for real-time mode
  "[R] Timer (custom)",
  "[R] Real-time",
  "[R] Real-time (paused)",
  "[R] Real-time (offset)",
  "[R] Real-time (custom)",
};
static const char* tick_name[] = {
  "[T] Smooth time",
  "[T] Tick time",
};
static const char* draw_branches_name[] = {
  "[B] Hide branches",
  "[B] Draw branches",
};
static const char* draw_clock_name[] = {
  "[C] Hide clock",
  "[C] Draw clock",
};
static const char* toggle_fullscreen_name[] = {
  "[F11] Full screen",
  "[F11] Exit full screen",
};
static const char* show_shortcuts_name = "[K] Hide shortcuts";

static std::vector<sf::Vertex> line_array;
static std::vector<sf::Vertex> point_array;
static std::vector<sf::Vertex> clock_face_array1;
static std::vector<sf::Vertex> clock_face_array2;
static sf::Vector2f rotH, rotM, rotS, rotT;
static float ratioH = 0.5f;
static float ratioM = std::sqrt(1.0f / 2.0f);
static float ratioS = std::sqrt(1.0f / 2.0f);
static float ratioT = std::sqrt(1.0f / 2.0f);
static float clock_ratio = std::sqrt(1.0f / 2.0f);
static float real_time = 0.0f;
static float shown_time = 0.0f;
static float paused_time = 0.0f;
static float pause_offset = 0.0f;
static float time_offset = 0.0f;
static float offsetH = 0.0f;
static float offsetM = 0.0f;
static float offsetS = 0.0f;
static float offsetT = 0.0f;
static int offset_hand = 0;
static int offset_mode = 0;
static int iter_diff = 0;
static int old_iters = max_iters;
static bool invert_iter_diff = false;
static bool timer_is_stopped = false;
static sf::Color color_scheme[max_iters];
static bool toggle_fullscreen = false;

#pragma warning(disable:26812)
static TimeType time_type = TimeType::REG;
#pragma warning(default:26812)
static bool show_hours = true;
static bool show_minutes = true;
static bool show_seconds = true;
static bool show_ticks = true;
static bool has_ticks = false;
static bool pause_time = false;
static bool use_real_time = true;
static bool use_tick = false;
static bool draw_branches = true;
static bool draw_clock = true;
static bool is_fullscreen = false;
static bool show_shortcuts = true;

struct Res
{
  Res(int id)
  {
    HRSRC src = ::FindResource(NULL, MAKEINTRESOURCE(id), RT_RCDATA);
    ptr = ::LockResource(::LoadResource(NULL, src));
    size = (size_t)::SizeofResource(NULL, src);
  }
  void* ptr;
  size_t size;
};

void FractalIter(sf::Vector2f pt, sf::Vector2f dir, int depth, bool h, bool m, bool s, bool t)
{
  const sf::Color& col = color_scheme[depth];
  if (depth == 0)
    point_array.emplace_back(pt, col);
  else
  {
    const sf::Vector2f dirT((dir.x * rotT.x - dir.y * rotT.y) * ratioT, (dir.y * rotT.x + dir.x * rotT.y) * ratioT);
    const sf::Vector2f dirS((dir.x * rotS.x - dir.y * rotS.y) * ratioS, (dir.y * rotS.x + dir.x * rotS.y) * ratioS);
    const sf::Vector2f dirM((dir.x * rotM.x - dir.y * rotM.y) * ratioM, (dir.y * rotM.x + dir.x * rotM.y) * ratioM);
    const sf::Vector2f dirH((dir.x * rotH.x - dir.y * rotH.y) * ratioH, (dir.y * rotH.x + dir.x * rotH.y) * ratioH);
    if (t) FractalIter(pt + dirT, dirT, depth - 1, h, m, s, t);
    if (s) FractalIter(pt + dirS, dirS, depth - 1, h, m, s, t);
    if (m) FractalIter(pt + dirM, dirM, depth - 1, h, m, s, t);
    if (h) FractalIter(pt + dirH, dirH, depth - 1, h, m, s, t);
    if (t) 
    {
      line_array.emplace_back(pt, col);
      line_array.emplace_back(pt + dirT, col);
    }
    if (s)
    {
      line_array.emplace_back(pt, col);
      line_array.emplace_back(pt + dirS, col);
    }
    if (m)
    {
      line_array.emplace_back(pt, col);
      line_array.emplace_back(pt + dirM, col);
    }
    if (h)
    {
      line_array.emplace_back(pt, col);
      line_array.emplace_back(pt + dirH, col);
    }
  }
}

INT WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, INT nShowCmd)
{
  //GL settings
  sf::ContextSettings settings;
  settings.depthBits = 24;
  settings.stencilBits = 8;
  settings.antialiasingLevel = 4;
  settings.majorVersion = 3;
  settings.minorVersion = 0;

  //Load the font
  sf::Font font;
  Res font_res(IDR_FONT);
  font.loadFromMemory(font_res.ptr, font_res.size);

  //Setup UI elements
  sf::Text clock_num;
  clock_num.setFont(font);
  clock_num.setFillColor(clock_face_color);

  sf::Text show_shortcuts_text;
  show_shortcuts_text.setFont(font);
  show_shortcuts_text.setFillColor(clock_face_color);
  show_shortcuts_text.setCharacterSize(22);
  show_shortcuts_text.setPosition(10, 10);
  sf::Text time_type_text;
  time_type_text.setFont(font);
  time_type_text.setFillColor(clock_face_color);
  time_type_text.setCharacterSize(22);
  time_type_text.setPosition(10, 40);
  sf::Text real_time_text;
  real_time_text.setFont(font);
  real_time_text.setFillColor(clock_face_color);
  real_time_text.setCharacterSize(22);
  real_time_text.setPosition(10, 70);
  sf::Text tick_text;
  tick_text.setFont(font);
  tick_text.setFillColor(clock_face_color);
  tick_text.setCharacterSize(22);
  tick_text.setPosition(10, 100);
  sf::Text draw_branches_text;
  draw_branches_text.setFont(font);
  draw_branches_text.setFillColor(clock_face_color);
  draw_branches_text.setCharacterSize(22);
  draw_branches_text.setPosition(10, 130);
  sf::Text draw_clock_text;
  draw_clock_text.setFont(font);
  draw_clock_text.setFillColor(clock_face_color);
  draw_clock_text.setCharacterSize(22);
  draw_clock_text.setPosition(10, 160);
  sf::Text toggle_fullscreen_text;
  toggle_fullscreen_text.setFont(font);
  toggle_fullscreen_text.setFillColor(clock_face_color);
  toggle_fullscreen_text.setCharacterSize(22);
  toggle_fullscreen_text.setPosition(10, 190);

  sf::Text show_hands_text;
  show_hands_text.setFont(font);
  show_hands_text.setFillColor(clock_face_color);
  show_hands_text.setCharacterSize(22);
  sf::Text select_hands_text;
  select_hands_text.setFont(font);
  select_hands_text.setFillColor(clock_face_color);
  select_hands_text.setCharacterSize(22);
  sf::Text time_offset_text;
  time_offset_text.setFont(font);
  time_offset_text.setFillColor(clock_face_color);
  time_offset_text.setCharacterSize(22);
  sf::Text change_offset_text;
  change_offset_text.setFont(font);
  change_offset_text.setFillColor(clock_face_color);
  change_offset_text.setCharacterSize(22);
  sf::Text reset_hand_offset_text;
  reset_hand_offset_text.setFont(font);
  reset_hand_offset_text.setFillColor(clock_face_color);
  reset_hand_offset_text.setCharacterSize(22);
  sf::Text reset_time_offset_text;
  reset_time_offset_text.setFont(font);
  reset_time_offset_text.setFillColor(clock_face_color);
  reset_time_offset_text.setCharacterSize(22);
  sf::Text pause_time_text;
  pause_time_text.setFont(font);
  pause_time_text.setFillColor(clock_face_color);
  pause_time_text.setCharacterSize(22);

  //Create the window
  sf::VideoMode screenSize = sf::VideoMode(window_w_init, window_h_init, 24);
  sf::RenderWindow window(screenSize, "Fractal Clock", sf::Style::Resize | sf::Style::Close, settings);
  window.setFramerateLimit(60);
  window.setActive(true);
  window.requestFocus();
  sf::Clock clock;

  //Main Loop
  clock.restart();
  while (window.isOpen())
  {
    float time_offset_delta = 0.0f;
    sf::Event event;
    while (window.pollEvent(event))
    {
      if (event.type == sf::Event::Closed)
      {
        window.close();
        break;
      }
      else if (event.type == sf::Event::KeyReleased)
      {
        offset_mode = 1 * event.key.shift + 2 * event.key.control + 4 * event.key.alt;
      }
      else if (event.type == sf::Event::KeyPressed)
      {
        offset_mode = 1 * event.key.shift + 2 * event.key.control + 4 * event.key.alt;
        #pragma warning(disable:26812)
        const sf::Keyboard::Key keycode = event.key.code;
        #pragma warning(default:26812)
        if (keycode == sf::Keyboard::Escape)
        {
          window.close();
          break;
        }
        else if (keycode == sf::Keyboard::Num1 || keycode == sf::Keyboard::Numpad1)
          if (event.key.shift)
            offset_hand = (offset_hand == 1 || !show_hours) ? 0 : 1;
          else
            show_hours = !show_hours;
        else if (keycode == sf::Keyboard::Num2 || keycode == sf::Keyboard::Numpad2)
          if (event.key.shift)
            offset_hand = (offset_hand == 2 || !show_minutes) ? 0 : 2;
          else
            show_minutes = !show_minutes;
        else if (keycode == sf::Keyboard::Num3 || keycode == sf::Keyboard::Numpad3)
          if (event.key.shift)
            offset_hand = (offset_hand == 3 || !show_seconds) ? 0 : 3;
          else
            show_seconds = !show_seconds;
        else if (keycode == sf::Keyboard::Num4 || keycode == sf::Keyboard::Numpad4)
          if (event.key.shift)
            offset_hand = (offset_hand == 4 || !show_ticks || !has_ticks) ? 0 : 4;
          else
            show_ticks = !show_ticks;
        else if (keycode == sf::Keyboard::Num0 || keycode == sf::Keyboard::Numpad0)
        {
          if (event.key.alt) {
            ratioH = 0.5f;
            ratioM = std::sqrt(1.0f / 2.0f);
            ratioS = std::sqrt(1.0f / 2.0f);
            ratioT = std::sqrt(1.0f / 2.0f);
            clock_ratio = std::sqrt(1.0f / 2.0f);
            offsetH = 0.0f;
            offsetM = 0.0f;
            offsetS = 0.0f;
            offsetT = 0.0f;
            paused_time = use_real_time ? real_time : 0.0f;
            pause_offset = 0.0f;
            time_offset = 0.0f;
            offset_hand = 0; // reset this only if Alt+0 was pressed
          }
          else {
            if (event.key.control)
            {
              if (offset_hand == 1 && show_hours)
                ratioH = 0.5f;
              else if (offset_hand == 2 && show_minutes)
                ratioM = std::sqrt(1.0f / 2.0f);
              else if (offset_hand == 3 && show_seconds)
                ratioS = std::sqrt(1.0f / 2.0f);
              else if (offset_hand == 4 && show_ticks && has_ticks)
                ratioT = std::sqrt(1.0f / 2.0f);
            }
            if (event.key.shift)
            {
              if (offset_hand == 1 && show_hours)
                offsetH = 0.0f;
              else if (offset_hand == 2 && show_minutes)
                offsetM = 0.0f;
              else if (offset_hand == 3 && show_seconds)
                offsetS = 0.0f;
              else if (offset_hand == 4 && show_ticks && has_ticks)
                offsetT = 0.0f;
            }
            if (!event.key.shift && !event.key.control)
            {
              paused_time = use_real_time ? real_time : 0.0f;
              pause_offset = 0.0f;
              time_offset = 0.0f;
            }
          }
          clock.restart();
        }
        else if (keycode == sf::Keyboard::Equal || keycode == sf::Keyboard::Add || keycode == sf::Keyboard::D || keycode == sf::Keyboard::Right)
        {
          if (offset_hand == 1 && show_hours)
            offsetH += hand_offset_modes[offset_mode];
          else if (offset_hand == 2 && show_minutes)
            offsetM += hand_offset_modes[offset_mode];
          else if (offset_hand == 3 && show_seconds)
            offsetS += hand_offset_modes[offset_mode];
          else if (offset_hand == 4 && show_ticks && has_ticks)
            offsetT += hand_offset_modes[offset_mode];
          else
            time_offset += time_offset_modes[offset_mode];
        }
        else if (keycode == sf::Keyboard::Hyphen || keycode == sf::Keyboard::Subtract || keycode == sf::Keyboard::A || keycode == sf::Keyboard::Left)
        {
          if (offset_hand == 1 && show_hours)
            offsetH -= hand_offset_modes[offset_mode];
          else if (offset_hand == 2 && show_minutes)
            offsetM -= hand_offset_modes[offset_mode];
          else if (offset_hand == 3 && show_seconds)
            offsetS -= hand_offset_modes[offset_mode];
          else if (offset_hand == 4 && show_ticks && has_ticks)
            offsetT -= hand_offset_modes[offset_mode];
          else
            time_offset -= time_offset_modes[offset_mode];
        }
        else if (keycode == sf::Keyboard::W || keycode == sf::Keyboard::Up)
        {
          if (event.key.control)
            clock_ratio = std::pow(clock_ratio, std::cbrt(std::cbrt(3.0f)));
          else if (offset_hand == 1 && show_hours)
            ratioH = std::pow(ratioH, event.key.shift ? std::cbrt(std::cbrt(1.0f / 3.0f)) : std::sqrt(std::sqrt(1.0f / 2.0f)));
          else if (offset_hand == 2 && show_minutes)
            ratioM = std::pow(ratioM, event.key.shift ? std::cbrt(std::cbrt(1.0f / 3.0f)) : std::sqrt(std::sqrt(1.0f / 2.0f)));
          else if (offset_hand == 3 && show_seconds)
            ratioS = std::pow(ratioS, event.key.shift ? std::cbrt(std::cbrt(1.0f / 3.0f)) : std::sqrt(std::sqrt(1.0f / 2.0f)));
          else if (offset_hand == 4 && show_ticks && has_ticks)
            ratioT = std::pow(ratioT, event.key.shift ? std::cbrt(std::cbrt(1.0f / 3.0f)) : std::sqrt(std::sqrt(1.0f / 2.0f)));
          else if (event.key.shift)
            clock_ratio = std::pow(clock_ratio, std::sqrt(std::sqrt(2.0f)));
          else
            --iter_diff;
        }
        else if (keycode == sf::Keyboard::S || keycode == sf::Keyboard::Down)
        {
          if (event.key.control)
            clock_ratio = std::pow(clock_ratio, std::cbrt(std::cbrt(1.0f / 3.0f)));
          else if (offset_hand == 1 && show_hours)
            ratioH = std::pow(ratioH, event.key.shift ? std::cbrt(std::cbrt(3.0f)) : std::sqrt(std::sqrt(2.0f)));
          else if (offset_hand == 2 && show_minutes)
            ratioM = std::pow(ratioM, event.key.shift ? std::cbrt(std::cbrt(3.0f)) : std::sqrt(std::sqrt(2.0f)));
          else if (offset_hand == 3 && show_seconds)
            ratioS = std::pow(ratioS, event.key.shift ? std::cbrt(std::cbrt(3.0f)) : std::sqrt(std::sqrt(2.0f)));
          else if (offset_hand == 4 && show_ticks && has_ticks)
            ratioT = std::pow(ratioT, event.key.shift ? std::cbrt(std::cbrt(3.0f)) : std::sqrt(std::sqrt(2.0f)));
          else if (event.key.shift)
            clock_ratio = std::pow(clock_ratio, std::sqrt(std::sqrt(1.0f / 2.0f)));
          else
            ++iter_diff;
        }
        else if (keycode == sf::Keyboard::P)
          pause_time = !pause_time;
        else if (keycode == sf::Keyboard::M)
          time_type = TimeType((time_type + 1) % TimeType::TIME_NUM);
        else if (keycode == sf::Keyboard::N)
          time_type = TimeType((time_type + TimeType::TIME_NUM - 1) % TimeType::TIME_NUM);
        else if (keycode == sf::Keyboard::R)
        {
          if (event.key.control)
          { // reset EVERYTHING
            timer_is_stopped = false;
            time_type = TimeType::REG;
            use_tick = false;
            toggle_fullscreen = false;
            show_shortcuts = true;
          } 
          if (event.key.shift || event.key.control)
          { // reset everything, but not really
            ratioH = 0.5f;
            ratioM = std::sqrt(1.0f / 2.0f);
            ratioS = std::sqrt(1.0f / 2.0f);
            ratioT = std::sqrt(1.0f / 2.0f);
            clock_ratio = std::sqrt(1.0f / 2.0f);
            paused_time = 0.0f;
            offset_hand = 0;
            offset_mode = 0;
            iter_diff = 0;
            old_iters = max_iters;
            invert_iter_diff = false;
            show_hours = true;
            show_minutes = true;
            show_seconds = true;
            show_ticks = true;
            pause_time = false;
            use_real_time = true;
            draw_branches = true;
            draw_clock = true;
          }
          if (!event.key.shift && !event.key.control)
          { // do what the R key is normally supposed to do
            use_real_time = !use_real_time;
            paused_time = use_real_time ? real_time : 0.0f;
          }
          offsetH = 0.0f;
          offsetM = 0.0f;
          offsetS = 0.0f;
          offsetT = 0.0f;
          pause_offset = 0.0f;
          time_offset = 0.0f;
          clock.restart();
        }
        else if (keycode == sf::Keyboard::T)
          use_tick = !use_tick;
        else if (keycode == sf::Keyboard::B)
          draw_branches = !draw_branches;
        else if (keycode == sf::Keyboard::C)
        {
          if (event.key.shift || event.key.control)
            clock_ratio = std::sqrt(1.0f / 2.0f);
          else
            draw_clock = !draw_clock;
        }
        else if (keycode == sf::Keyboard::F11)
          toggle_fullscreen = true;
        else if (keycode == sf::Keyboard::K)
          show_shortcuts = !show_shortcuts;
      }
      else if (event.type == sf::Event::Resized)
      {
        screenSize.width = event.size.width;
        screenSize.height = event.size.height;
        window.setView(sf::View(sf::FloatRect(0.0f, 0.0f, (float)screenSize.width, (float)screenSize.height)));
      }
    }

    //Initialize all variables that change between timekeeping systems
    float secondsPerFullCycle, secondsPerNewHour, secondsPerNewMinute, secondsPerNewSecond, secondsPerNewTick, startOffset;
    int numberAmount, linesBetweenNumbers;

    switch (time_type)
    {
    case REG:
      secondsPerNewTick = 0.0f;
      secondsPerNewSecond = 1.0f;
      secondsPerNewMinute = 60.0f;
      secondsPerNewHour = 3600.0f;
      secondsPerFullCycle = 43200.0f;
      startOffset = 0.0f;
      numberAmount = 12;
      linesBetweenNumbers = 5;
      break;
    case REG_24:
      secondsPerNewTick = 0.0f;
      secondsPerNewSecond = 1.0f;
      secondsPerNewMinute = 60.0f;
      secondsPerNewHour = 3600.0f;
      secondsPerFullCycle = 86400.0f;
      startOffset = PI;
      numberAmount = 24;
      linesBetweenNumbers = 5;
      break;
    case DEC:
      secondsPerNewTick = 0.0f;
      secondsPerNewSecond = 0.864f;
      secondsPerNewMinute = 86.4f;
      secondsPerNewHour = 8640.0f;
      secondsPerFullCycle = 86400.0f;
      startOffset = 0.0f;
      numberAmount = 10;
      linesBetweenNumbers = 10;
      break;
    case HEX:
      secondsPerNewTick = 1.318359375f;
      secondsPerNewSecond = 21.09375f;
      secondsPerNewMinute = 337.5f;
      secondsPerNewHour = 5400.0f;
      secondsPerFullCycle = 86400.0f;
      startOffset = 0.0f;
      numberAmount = 16;
      linesBetweenNumbers = 4;
      break;
    case CRT:
      secondsPerNewTick = 0.0f;
      secondsPerNewSecond = 1.318359375f;
      secondsPerNewMinute = 84.375f;
      secondsPerNewHour = 5400.0f;
      secondsPerFullCycle = 86400.0f;
      startOffset = 0.0f;
      numberAmount = 16;
      linesBetweenNumbers = 4;
      break;
    case DOZ:
      secondsPerNewTick = 25.0f / 12.0f;
      secondsPerNewSecond = 25.0f;
      secondsPerNewMinute = 300.0f;
      secondsPerNewHour = 3600.0f;
      secondsPerFullCycle = 43200.0f;
      startOffset = 0.0f;
      numberAmount = 12;
      linesBetweenNumbers = 4;
      break;
    case SEN_3:
      secondsPerNewTick = 0.0f;
      secondsPerNewSecond = 50.0f / 27.0f;
      secondsPerNewMinute = 200.0f / 3.0f;
      secondsPerNewHour = 2400.0f;
      secondsPerFullCycle = 86400.0f;
      startOffset = 0.0f;
      numberAmount = 6;
      linesBetweenNumbers = 6;
      break;
    case SEN_4:
      secondsPerNewTick = 50.0f / 27.0f;
      secondsPerNewSecond = 200.0f / 3.0f;
      secondsPerNewMinute = 2400.0f;
      secondsPerNewHour = 14400.0f;
      secondsPerFullCycle = 86400.0f;
      startOffset = 0.0f;
      numberAmount = 6;
      linesBetweenNumbers = 6;
      break;
    }

    const int lineAmount = numberAmount * linesBetweenNumbers;
    has_ticks = secondsPerNewTick != 0.0f;

    //Calculate maximum iterations
    int iters = max_iters;
    const int handCount = int(show_hours) + int(show_minutes) + int(show_seconds) + int(has_ticks && show_ticks);
    if (handCount == 3)
      iters = iters - 3;
    if (handCount == 4)
      iters = iters - 6;

    //Calculate *actual* iterations, becuase an unidentified lifeform decided being able to tweak this number would be a good idea
    const int old_iter_diff = iter_diff;
    // If the iteration difference has gone out of its intended boundaries, put it back into [2, iters]
    if (iter_diff <= 0)
    { // Assume the user wants to keep the iteration count high, count from top
      iter_diff = 0;
      invert_iter_diff = false;
    }
    if (iters - iter_diff <= 2)
    { // Assume the user wants to keep the iteration count low, count from 0
      iter_diff = iters - 2;
      invert_iter_diff = true;
    }
    // Note that the iteration difference should always be changed when its bounds won't be, and vice versa
    // Edge case here is "we now have different bounds for iteration count than before"
    const bool boundsChanged = iters != old_iters;  // If we do, and if the iteration count needs to be counted from 0
    if (boundsChanged && invert_iter_diff)          // Set iter_diff value so that the final difference stays the same
      iter_diff = old_iter_diff + iters - old_iters;   // iters - iter_diff == old_iters - old_iter_diff
    old_iters = iters;
    iters = iters - iter_diff;

    //Get the system time
    FILETIME fileTime;
    SYSTEMTIME systemTime;
    SYSTEMTIME localTime;
    GetSystemTimeAsFileTime(&fileTime);
    FileTimeToSystemTime(&fileTime, &systemTime);
    SystemTimeToTzSpecificLocalTime(NULL, &systemTime, &localTime);
    real_time = float(localTime.wMilliseconds) / 1000.0f;
    real_time += float(localTime.wSecond);
    real_time += float(localTime.wMinute) * 60.0f;
    real_time += float(localTime.wHour) * 3600.0f;

    //Get the time without accounting for time/pause offsets
    float current_time = use_real_time ? real_time : clock.getElapsedTime().asSeconds();
    timer_is_stopped = (use_real_time || !timer_is_stopped) ? false : pause_time;
    if (timer_is_stopped)
    {
        current_time = 0.0f;
        clock.restart();
    }

    //Update the variables used for pausing time
    if (pause_time)
      pause_offset = paused_time - current_time;
    else
      paused_time = shown_time - time_offset;
    shown_time = current_time;

    //Cap the offsets to reduce precision errors
    time_offset = std::fmodf(time_offset, secondsPerFullCycle);
    pause_offset = std::fmodf(pause_offset, secondsPerFullCycle);

    //Change the time according to the time/pause offsets
    shown_time += time_offset + pause_offset;

    //Move the time slightly to create a ticking animation
    float animated_time = shown_time;
    if (use_tick)
    {
        const float secondsPerSmallestTimeStep = has_ticks ? secondsPerNewTick : secondsPerNewSecond;
        static const float a = 30.0f;
        static const float b = 14.0f;
        const float x = std::fmodf(animated_time, secondsPerSmallestTimeStep);
        const float y = (1.0f - std::cos(a * x) * std::exp(-b * x)) * secondsPerSmallestTimeStep;
        animated_time = animated_time - x + y;
    }

    //Calculate hand angles
    const float ticks = has_ticks ? std::fmodf(animated_time, secondsPerNewSecond) * 2.0f * PI / secondsPerNewSecond + std::fmodf(startOffset + offsetT, 2.0f * PI) : 0.0f;
    const float seconds = std::fmodf(animated_time, secondsPerNewMinute) * 2.0f * PI / secondsPerNewMinute + std::fmodf(startOffset + offsetS, 2.0f * PI);
    const float minutes = std::fmodf(animated_time, secondsPerNewHour) * 2.0f * PI / secondsPerNewHour + std::fmodf(startOffset + offsetM, 2.0f * PI);
    const float hours = std::fmodf(animated_time, secondsPerFullCycle) * 2.0f * PI / secondsPerFullCycle + std::fmodf(startOffset + offsetH, 2.0f * PI);

    //Update the clock
    //const float ratio = std::max(std::max(std::max(ratioH, ratioM), ratioS), has_ticks ? ratioT : 0.0f);
    //const float start_mag = std::min(screenSize.width, screenSize.height) * 0.5f * (1.0f - ratio) / ratio;
    const float start_mag = std::min(screenSize.width, screenSize.height) * 0.5f * (1.0f - clock_ratio) / clock_ratio;

    rotH = sf::Vector2f(std::cos(hours), std::sin(hours));
    rotM = sf::Vector2f(std::cos(minutes), std::sin(minutes));
    rotS = sf::Vector2f(std::cos(seconds), std::sin(seconds));
    rotT = has_ticks ? sf::Vector2f(std::cos(ticks), std::sin(ticks)) : sf::Vector2f();
    const sf::Vector2f pt(float(screenSize.width)*0.5f, float(screenSize.height)*0.5f);
    const sf::Vector2f dir(0.0f, -start_mag);

    //Update the clock face
    clock_face_array1.clear();
    clock_face_array2.clear();
    for (int i = 0; i < lineAmount; ++i)
    {
      const float ang = float(i) * 2.0f * PI / (float)lineAmount + startOffset;
      const sf::Vector2f v(std::sin(ang), -std::cos(ang));
      const bool is_hour = (i % linesBetweenNumbers == 0);
      std::vector<sf::Vertex>& clock_face_array = (is_hour ? clock_face_array1 : clock_face_array2);
      const float inner_rad = (is_hour ? 0.9f : 0.95f);
      clock_face_array.emplace_back(pt + v * start_mag * inner_rad, clock_face_color);
      clock_face_array.emplace_back(pt + v * start_mag * 1.0f, clock_face_color);
    }

    //Update the colors
    const float r1 = std::sin(real_time * 0.017f)*0.5f + 0.5f;
    const float r2 = std::sin(real_time * 0.011f)*0.5f + 0.5f;
    const float r3 = std::sin(real_time * 0.003f)*0.5f + 0.5f;
    for (int i = 0; i < iters; ++i)
    {
      const float a = float(i) / float(iters - 1);
      const float h = std::fmodf(r2 + 0.5f*a, 1.0f);
      const float s = 0.5f + 0.5f * r3 - 0.5f*(1.0f - a);
      const float v = 0.3f + 0.5f * r1;
      if (i == 0)
      {
        color_scheme[i] = FromHSV(h, 1.0f, 1.0f);
        color_scheme[i].a = 128;
      }
      else if (i == iters - 1 && draw_clock)
        color_scheme[i] = clock_face_color;
      else
      {
        color_scheme[i] = FromHSV(h, s, v);
        color_scheme[i].a = 255; //128;
      }
    }

    //Update the fractal
    line_array.clear();
    point_array.clear();
    FractalIter(pt, dir, iters - 1, show_hours, show_minutes, show_seconds, has_ticks && show_ticks);

    //Clear the screen
    window.clear(bgnd_color);

    //Draw the fractal branches
    if (draw_branches)
    {
      glEnable(GL_LINE_SMOOTH);
      glLineWidth(2.0f);
      if (!draw_clock)
        window.draw(line_array.data(), line_array.size(), sf::PrimitiveType::Lines);
      else
        window.draw(line_array.data(), line_array.size() - 2ll * handCount, sf::PrimitiveType::Lines);
    }

    //Draw the final fractal in a brighter color
    glEnable(GL_POINT_SMOOTH);
    glPointSize(1.0f);
    window.draw(point_array.data(), point_array.size(), sf::PrimitiveType::Points);

    //Draw the clock
    if (draw_clock)
    {
      //Draw the clock face lines
      glEnable(GL_LINE_SMOOTH);
      glLineWidth(4.0f);
      window.draw(clock_face_array1.data(), clock_face_array1.size(), sf::PrimitiveType::Lines);
      glLineWidth(2.0f);
      window.draw(clock_face_array2.data(), clock_face_array2.size(), sf::PrimitiveType::Lines);
      
      //Draw the clock face numbers
      for (int i = 0; i < numberAmount; ++i)
      {
        const float ang = float(i) * 2.0f * PI / (float)numberAmount + startOffset;
        const sf::Vector2f v(std::sin(ang), -std::cos(ang));
        std::string num_str;
        switch (time_type) {
          case HEX: case CRT: num_str = std::string(1, i < 10 ? ('0' + i) : ('A' + i - 10)); break;
          case DOZ: num_str = i == 10 ? "X" : i == 11 ? "E" : i == 0 ? "10" : std::to_string(i); break;
          case REG: num_str = i == 0 ? "12" : std::to_string(i); break;
          default: num_str = std::to_string(i); break;
        }
        clock_num.setString(num_str);
        clock_num.setCharacterSize(uint32_t(time_type == TimeType::REG_24 ? start_mag * 0.12f : start_mag * 0.18f));

        const sf::FloatRect bounds = clock_num.getLocalBounds();
        clock_num.setOrigin(bounds.width * 0.5f, bounds.height * 0.85f);
        clock_num.setPosition(pt + v * start_mag * 0.8f);
        window.draw(clock_num);
      }

      //Draw the clock hands
      const sf::Vertex* startIndex = line_array.data() + line_array.size() - 2ll * handCount;
      if (show_ticks)
      {
        glLineWidth(1.0f);
        window.draw(startIndex, 2, sf::PrimitiveType::Lines);
      }
      if (show_seconds)
      {
        glLineWidth(2.0f);
        window.draw(startIndex + 2ll * (has_ticks && show_ticks), 2, sf::PrimitiveType::Lines);
      }
      if (show_minutes)
      {
        glLineWidth(4.0f);
        window.draw(startIndex + 2ll * (has_ticks && show_ticks) + 2ll * show_seconds, 2, sf::PrimitiveType::Lines);
      }
      if (show_hours)
      {
        glLineWidth(5.0f);
        window.draw(startIndex + 2ll * (has_ticks && show_ticks) + 2ll * show_seconds + 2ll * show_minutes, 2, sf::PrimitiveType::Lines);
      }
    }

    //Draw UI elements
    if (show_shortcuts)
    {
      const bool time_is_custom = (offsetH != 0.0f || offsetM != 0.0f || offsetS != 0.0f || offsetT != 0.0f);
      const bool time_is_offset = (time_offset != 0.0f || pause_offset != 0.0f) && !pause_time;
      const bool time_is_paused = pause_time;
      show_shortcuts_text.setString(show_shortcuts_name);
      window.draw(show_shortcuts_text);
      time_type_text.setString(time_type_name[time_type]);
      window.draw(time_type_text);
      real_time_text.setString(real_time_name[4 * use_real_time + (time_is_custom ? 3 : time_is_offset ? 2 : time_is_paused)]);
      window.draw(real_time_text);
      tick_text.setString(tick_name[use_tick]);
      window.draw(tick_text);
      draw_branches_text.setString(draw_branches_name[draw_branches]);
      window.draw(draw_branches_text);
      draw_clock_text.setString(draw_clock_name[draw_clock]);
      window.draw(draw_clock_text);
      toggle_fullscreen_text.setString(toggle_fullscreen_name[is_fullscreen]);
      window.draw(toggle_fullscreen_text);

      show_hands_text.setString(show_hands_name[has_ticks]);
      show_hands_text.setPosition(screenSize.width - show_hands_text.getGlobalBounds().width - 10, 10);
      window.draw(show_hands_text);
      select_hands_text.setString(offset_hand ? deselect_hands_name[offset_hand - 1] : select_hands_name[has_ticks]);
      select_hands_text.setPosition(screenSize.width - select_hands_text.getGlobalBounds().width - 10, 40);
      window.draw(select_hands_text);
      time_offset_text.setString(offset_hand ? hand_offset_name[offset_mode] : time_offset_name[offset_mode]);
      time_offset_text.setPosition(screenSize.width - time_offset_text.getGlobalBounds().width - 10, 70);
      window.draw(time_offset_text);
      change_offset_text.setString(change_offset_name);
      change_offset_text.setPosition(screenSize.width - change_offset_text.getGlobalBounds().width - 10, 100);
      window.draw(change_offset_text);
      reset_hand_offset_text.setString(reset_hand_offset_name);
      reset_hand_offset_text.setPosition(screenSize.width - reset_hand_offset_text.getGlobalBounds().width - 10, 130);
      window.draw(reset_hand_offset_text);
      reset_time_offset_text.setString(reset_time_offset_name[use_real_time]);
      reset_time_offset_text.setPosition(screenSize.width - reset_time_offset_text.getGlobalBounds().width - 10, 160);
      window.draw(reset_time_offset_text);
      pause_time_text.setString(pause_time_name[pause_time]);
      pause_time_text.setPosition(screenSize.width - pause_time_text.getGlobalBounds().width - 10, 190);
      window.draw(pause_time_text);
    }

    //Flip the screen buffer
    window.display();

    //Toggle full-screen if needed
    if (toggle_fullscreen)
    {
      toggle_fullscreen = false;
      is_fullscreen = !is_fullscreen;
      if (is_fullscreen)
      {
        window.close();
        screenSize = sf::VideoMode::getDesktopMode();
        window.create(screenSize, "Fractal Clock", sf::Style::Fullscreen, settings);
      }
      else
      {
        window.close();
        screenSize = sf::VideoMode(window_w_init, window_h_init, 24);
        window.create(screenSize, "Fractal Clock", sf::Style::Resize | sf::Style::Close, settings);
      }
      window.setMouseCursorVisible(!is_fullscreen);
    }
  }
  return 0;
}
