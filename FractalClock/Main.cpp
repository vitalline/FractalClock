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
static const char* show_hands_name[] = {
  "[1-3] Show/Hide clock hands",
  "[1-4] Show/Hide clock hands",
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
static const char* change_offset_name = "[Shift/Ctrl/Alt] Offset step";
static const char* reset_offset_name = "[0] Reset time offset";
static const char* pause_time_name[] = {
  "[P] Pause time",
  "[P] Resume time",
};
static const char* real_time_name[] = {
  "[R] Timer",
  "[R] Timer (offset)",
  "[R] Real-time",
  "[R] Real-time (offset)",
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
static float real_time = 0.0f;
static float shown_time = 0.0f;
static float paused_time = 0.0f;
static float pause_offset = 0.0f;
static float time_offset = 0.0f;
static int time_offset_mode = 0;
static sf::Color color_scheme[max_iters];
static bool toggle_fullscreen = false;

#pragma warning(disable:26812)
static TimeType time_type = TimeType::REG;
#pragma warning(default:26812)
static bool show_hours = true;
static bool show_minutes = true;
static bool show_seconds = true;
static bool show_ticks = true;
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
  sf::Text real_time_text;
  real_time_text.setFont(font);
  real_time_text.setFillColor(clock_face_color);
  real_time_text.setCharacterSize(22);
  real_time_text.setPosition(10, 40);
  sf::Text tick_text;
  tick_text.setFont(font);
  tick_text.setFillColor(clock_face_color);
  tick_text.setCharacterSize(22);
  tick_text.setPosition(10, 70);
  sf::Text draw_branches_text;
  draw_branches_text.setFont(font);
  draw_branches_text.setFillColor(clock_face_color);
  draw_branches_text.setCharacterSize(22);
  draw_branches_text.setPosition(10, 100);
  sf::Text draw_clock_text;
  draw_clock_text.setFont(font);
  draw_clock_text.setFillColor(clock_face_color);
  draw_clock_text.setCharacterSize(22);
  draw_clock_text.setPosition(10, 130);
  sf::Text toggle_fullscreen_text;
  toggle_fullscreen_text.setFont(font);
  toggle_fullscreen_text.setFillColor(clock_face_color);
  toggle_fullscreen_text.setCharacterSize(22);
  toggle_fullscreen_text.setPosition(10, 160);

  sf::Text time_type_text;
  time_type_text.setFont(font);
  time_type_text.setFillColor(clock_face_color);
  time_type_text.setCharacterSize(22);
  sf::Text show_hands_text;
  show_hands_text.setFont(font);
  show_hands_text.setFillColor(clock_face_color);
  show_hands_text.setCharacterSize(22);
  sf::Text time_offset_text;
  time_offset_text.setFont(font);
  time_offset_text.setFillColor(clock_face_color);
  time_offset_text.setCharacterSize(22);
  sf::Text change_offset_text;
  change_offset_text.setFont(font);
  change_offset_text.setFillColor(clock_face_color);
  change_offset_text.setCharacterSize(22);
  sf::Text reset_offset_text;
  reset_offset_text.setFont(font);
  reset_offset_text.setFillColor(clock_face_color);
  reset_offset_text.setCharacterSize(22);
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
        time_offset_mode = 1 * event.key.shift + 2 * event.key.control + 4 * event.key.alt;
      }
      else if (event.type == sf::Event::KeyPressed)
      {
        time_offset_mode = 1 * event.key.shift + 2 * event.key.control + 4 * event.key.alt;
        #pragma warning(disable:26812)
        const sf::Keyboard::Key keycode = event.key.code;
        #pragma warning(default:26812)
        if (keycode == sf::Keyboard::Escape)
        {
          window.close();
          break;
        }
        else if (keycode == sf::Keyboard::Num1 || keycode == sf::Keyboard::Numpad1)
          show_hours = !show_hours;
        else if (keycode == sf::Keyboard::Num2 || keycode == sf::Keyboard::Numpad2)
          show_minutes = !show_minutes;
        else if (keycode == sf::Keyboard::Num3 || keycode == sf::Keyboard::Numpad3)
          show_seconds = !show_seconds;
        else if (keycode == sf::Keyboard::Num4 || keycode == sf::Keyboard::Numpad4)
          show_ticks = !show_ticks;
        else if (keycode == sf::Keyboard::Num0 || keycode == sf::Keyboard::Numpad0)
        {
          paused_time = real_time;
          time_offset = 0;
          pause_offset = 0;
        }
        else if (keycode == sf::Keyboard::Equal || keycode == sf::Keyboard::Add)
          time_offset += time_offset_modes[time_offset_mode];
        else if (keycode == sf::Keyboard::Hyphen || keycode == sf::Keyboard::Subtract)
          time_offset -= time_offset_modes[time_offset_mode];
        else if (keycode == sf::Keyboard::P)
          pause_time = !pause_time;
        else if (keycode == sf::Keyboard::M)
          time_type = TimeType((time_type + 1) % TimeType::TIME_NUM);
        else if (keycode == sf::Keyboard::N)
          time_type = TimeType((time_type + TimeType::TIME_NUM - 1) % TimeType::TIME_NUM);
        else if (keycode == sf::Keyboard::R)
        {
          use_real_time = !use_real_time;
          pause_time = false;
          time_offset = 0;
          pause_offset = 0;
          clock.restart();
        }
        else if (keycode == sf::Keyboard::T)
          use_tick = !use_tick;
        else if (keycode == sf::Keyboard::B)
          draw_branches = !draw_branches;
        else if (keycode == sf::Keyboard::C)
          draw_clock = !draw_clock;
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

    switch (time_type) {
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
      secondsPerNewSecond = 0.32958984375f;
      secondsPerNewMinute = 21.09375f;
      secondsPerNewHour = 1350.0f;
      secondsPerFullCycle = 43200.0f;
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
    const bool hasTicks = secondsPerNewTick != 0.0f;

    //Calculate maximum iterations
    int iters = max_iters;
    const int handCount = int(show_hours) + int(show_minutes) + int(show_seconds) + int(hasTicks && show_ticks);
    if (handCount == 3)
      iters = max_iters - 3;
    if (handCount == 4)
      iters = max_iters - 6;

    //Get the time
    real_time = 0.0f;
    if (use_real_time)
    {
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
    }
    else
      real_time = clock.getElapsedTime().asSeconds();

    //Update the variables used for pausing time
    if (pause_time)
      pause_offset = paused_time - real_time;
    else
      paused_time = shown_time - time_offset;
    shown_time = real_time;

    //Cap the offsets to reduce precision errors
    time_offset = std::fmodf(time_offset, secondsPerFullCycle);
    pause_offset = std::fmodf(pause_offset, secondsPerFullCycle);

    //Move the time according to the offset and/or paused time
    shown_time += time_offset + pause_offset;

    //Move the time slightly to create a ticking animation
    float animated_time = shown_time;
    if (use_tick)
    {
        const float secondsPerSmallestTimeStep = hasTicks ? secondsPerNewTick : secondsPerNewSecond;
        static const float a = 30.0f;
        static const float b = 14.0f;
        const float x = std::fmodf(animated_time, secondsPerSmallestTimeStep);
        const float y = (1.0f - std::cos(a * x) * std::exp(-b * x)) * secondsPerSmallestTimeStep;
        animated_time = animated_time - x + y;
    }

    //Calculate hand angles
    const float ticks = hasTicks ? std::fmodf(animated_time, secondsPerNewSecond) * 2.0f * PI / secondsPerNewSecond + startOffset : -1.0f;
    const float seconds = std::fmodf(animated_time, secondsPerNewMinute) * 2.0f * PI / secondsPerNewMinute + startOffset;
    const float minutes = std::fmodf(animated_time, secondsPerNewHour) * 2.0f * PI / secondsPerNewHour + startOffset;
    const float hours = std::fmodf(animated_time, secondsPerFullCycle) * 2.0f * PI / secondsPerFullCycle + startOffset;

    //Update the clock
    const float ratio = std::max(std::max(std::max(ratioH, ratioM), ratioS), ratioT);
    const float start_mag = std::min(screenSize.width, screenSize.height) * 0.5f * (1.0f - ratio) / ratio;

    rotH = sf::Vector2f(std::cos(hours), std::sin(hours));
    rotM = sf::Vector2f(std::cos(minutes), std::sin(minutes));
    rotS = sf::Vector2f(std::cos(seconds), std::sin(seconds));
    rotT = ticks == -1.0f ? sf::Vector2f() : sf::Vector2f(std::cos(ticks), std::sin(ticks));
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
    FractalIter(pt, dir, iters - 1, show_hours, show_minutes, show_seconds, hasTicks && show_ticks);

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
        window.draw(startIndex + 2ll * (hasTicks && show_ticks), 2, sf::PrimitiveType::Lines);
      }
      if (show_minutes)
      {
        glLineWidth(4.0f);
        window.draw(startIndex + 2ll * (hasTicks && show_ticks) + 2ll * show_seconds, 2, sf::PrimitiveType::Lines);
      }
      if (show_hours)
      {
        glLineWidth(5.0f);
        window.draw(startIndex + 2ll * (hasTicks && show_ticks) + 2ll * show_seconds + 2ll * show_minutes, 2, sf::PrimitiveType::Lines);
      }
    }

    //Draw UI elements
    if (show_shortcuts)
    {
      show_shortcuts_text.setString(show_shortcuts_name);
      window.draw(show_shortcuts_text);
      real_time_text.setString(real_time_name[2 * use_real_time + (time_offset != 0.0f || pause_offset != 0.0f)]);
      window.draw(real_time_text);
      tick_text.setString(tick_name[use_tick]);
      window.draw(tick_text);
      draw_branches_text.setString(draw_branches_name[draw_branches]);
      window.draw(draw_branches_text);
      draw_clock_text.setString(draw_clock_name[draw_clock]);
      window.draw(draw_clock_text);
      toggle_fullscreen_text.setString(toggle_fullscreen_name[is_fullscreen]);
      window.draw(toggle_fullscreen_text);

      time_type_text.setString(time_type_name[time_type]);
      time_type_text.setPosition(screenSize.width - time_type_text.getGlobalBounds().width - 10, 10);
      window.draw(time_type_text);
      show_hands_text.setString(show_hands_name[hasTicks]);
      show_hands_text.setPosition(screenSize.width - show_hands_text.getGlobalBounds().width - 10, 40);
      window.draw(show_hands_text);
      time_offset_text.setString(time_offset_name[time_offset_mode]);
      time_offset_text.setPosition(screenSize.width - time_offset_text.getGlobalBounds().width - 10, 70);
      window.draw(time_offset_text);
      change_offset_text.setString(change_offset_name);
      change_offset_text.setPosition(screenSize.width - change_offset_text.getGlobalBounds().width - 10, 100);
      window.draw(change_offset_text);
      reset_offset_text.setString(reset_offset_name);
      reset_offset_text.setPosition(screenSize.width - reset_offset_text.getGlobalBounds().width - 10, 130);
      window.draw(reset_offset_text);
      pause_time_text.setString(pause_time_name[pause_time]);
      pause_time_text.setPosition(screenSize.width - pause_time_text.getGlobalBounds().width - 10, 160);
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
