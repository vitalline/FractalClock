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
enum ClockType {
  HM, MS, ST, HMS, MST, HMST, CLOCK_NUM
};
enum TimeType {
  REG, REG_24, SEN_3, SEN_4, DEC, DOZ, HEX, CRT, TIME_NUM
};
static const char* clock_type_name[] = {
  "[M] Hours, Minutes",
  "[M] Minutes, Seconds",
  "[M] Seconds, Ticks",
  "[M] Hours, Minutes, Seconds",
  "[M] Minutes, Seconds, Ticks",
  "[M] Hrs, Mins, Secs, Ticks",
};
static const char* time_type_name[] = {
  "[N] Regular Clock",
  "[N] 24-hour Clock",
  "[N] Heximal Clock",
  "[N] Heximal Clock (4 hands)",
  "[N] Decimal Clock",
  "[N] Dozenal Clock",
  "[N] Hexadecimal Clock",
  "[N] Creata Standard Time",
};
static const char* realtime_name[] = {
  "[R] Timer",
  "[R] Real-time",
};
static const char* tick_name[] = {
  "[T] Smooth Time",
  "[T] Tick Time",
};
static const char* draw_branches_name[] = {
  "[B] Hide Branches",
  "[B] Draw Branches",
};
static const char* draw_clock_name[] = {
  "[C] Hide Clock",
  "[C] Draw Clock",
};
static const char* toggle_fullscreen_name[] = {
  "[F11] Full screen",
  "[F11] Exit full screen",
};
static const char* show_shortcuts_name = "[K] Hide keyboard shortcuts";

static std::vector<sf::Vertex> line_array;
static std::vector<sf::Vertex> point_array;
static std::vector<sf::Vertex> clock_face_array1;
static std::vector<sf::Vertex> clock_face_array2;
static sf::Vector2f rotH, rotM, rotS, rotT;
static float ratioH = 0.5f;
static float ratioM = std::sqrt(1.0f / 2.0f);
static float ratioS = std::sqrt(1.0f / 2.0f);
static float ratioT = std::sqrt(1.0f / 2.0f);
static sf::Color color_scheme[max_iters];
static bool toggle_fullscreen = false;

#pragma warning(disable:26812)
static ClockType clock_type = ClockType::HMS;
static ClockType old_clock_type = clock_type;
static TimeType time_type = TimeType::REG;
#pragma warning(default:26812)
static bool use_realtime = true;
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
  sf::Text clock_type_text;
  clock_type_text.setFont(font);
  clock_type_text.setFillColor(clock_face_color);
  clock_type_text.setCharacterSize(22);
  clock_type_text.setPosition(10, 40);
  sf::Text time_type_text;
  time_type_text.setFont(font);
  time_type_text.setFillColor(clock_face_color);
  time_type_text.setCharacterSize(22);
  time_type_text.setPosition(10, 70);
  sf::Text realtime_text;
  realtime_text.setFont(font);
  realtime_text.setFillColor(clock_face_color);
  realtime_text.setCharacterSize(22);
  realtime_text.setPosition(10, 100);
  sf::Text tick_text;
  tick_text.setFont(font);
  tick_text.setFillColor(clock_face_color);
  tick_text.setCharacterSize(22);
  tick_text.setPosition(10, 130);
  sf::Text draw_branches_text;
  draw_branches_text.setFont(font);
  draw_branches_text.setFillColor(clock_face_color);
  draw_branches_text.setCharacterSize(22);
  draw_branches_text.setPosition(10, 160);
  sf::Text draw_clock_text;
  draw_clock_text.setFont(font);
  draw_clock_text.setFillColor(clock_face_color);
  draw_clock_text.setCharacterSize(22);
  draw_clock_text.setPosition(10, 190);
  sf::Text toggle_fullscreen_text;
  toggle_fullscreen_text.setFont(font);
  toggle_fullscreen_text.setFillColor(clock_face_color);
  toggle_fullscreen_text.setCharacterSize(22);
  toggle_fullscreen_text.setPosition(10, 220);

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
    sf::Event event;
    while (window.pollEvent(event))
    {
      if (event.type == sf::Event::Closed)
      {
        window.close();
        break;
      }
      else if (event.type == sf::Event::KeyPressed)
      {
        #pragma warning(disable:26812)
        const sf::Keyboard::Key keycode = event.key.code;
        #pragma warning(default:26812)
        if (keycode == sf::Keyboard::Escape)
        {
          window.close();
          break;
        }
        else if (keycode == sf::Keyboard::M) {
          clock_type = ClockType((clock_type + 1) % ClockType::CLOCK_NUM);
          if (time_type != SEN_4 && time_type != DOZ && time_type != HEX)
            while (clock_type == ClockType::HMST || clock_type == ClockType::MST || clock_type == ClockType::ST)
              clock_type = ClockType((clock_type + 1) % ClockType::CLOCK_NUM);
          old_clock_type = clock_type;
        }
        else if (keycode == sf::Keyboard::N)
          time_type = TimeType((time_type + 1) % TimeType::TIME_NUM);
        else if (keycode == sf::Keyboard::R)
        {
          use_realtime = !use_realtime;
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

        //Save state of clock if the new one does not have ticks (4th hand) 
        if (time_type != SEN_4 && time_type != DOZ && time_type != HEX)
        {
          if (clock_type == ClockType::HMST)
          {
            old_clock_type = clock_type;
            clock_type = ClockType::HMS;
          }
          else if (clock_type == ClockType::MST || clock_type == ClockType::ST)
          {
            old_clock_type = clock_type;
            clock_type = ClockType::MS;
          }
        }
        else
          clock_type = old_clock_type;
      }
      else if (event.type == sf::Event::Resized)
      {
        screenSize.width = event.size.width;
        screenSize.height = event.size.height;
        window.setView(sf::View(sf::FloatRect(0.0f, 0.0f, (float)screenSize.width, (float)screenSize.height)));
      }
    }

    //Calculate maximum iterations
    int iters = max_iters;
    if (clock_type == ClockType::HMS || clock_type == ClockType::MST)
        iters = max_iters - 3;
    if (clock_type == ClockType::HMST)
        iters = max_iters - 6;

    //Get the time
    float cur_time = 0.0f;
    if (use_realtime)
    {
      FILETIME fileTime;
      SYSTEMTIME systemTime;
      SYSTEMTIME localTime;
      GetSystemTimeAsFileTime(&fileTime);
      FileTimeToSystemTime(&fileTime, &systemTime);
      SystemTimeToTzSpecificLocalTime(NULL, &systemTime, &localTime);
      cur_time = float(localTime.wMilliseconds) / 1000.0f;
      cur_time += float(localTime.wSecond);
      cur_time += float(localTime.wMinute) * 60.0f;
      cur_time += float(localTime.wHour) * 3600.0f;
    }
    else
      cur_time = clock.getElapsedTime().asSeconds();

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
    
    //Move the time slightly to create a ticking animation
    if (use_tick)
    {
      const float secondsPerSmallestTimeStep = secondsPerNewTick == 0.0f ? secondsPerNewSecond : secondsPerNewTick;
      static const float a = 30.0f;
      static const float b = 14.0f;
      const float x = std::fmodf(cur_time, secondsPerSmallestTimeStep);
      const float y = (1.0f - std::cos(a*x)*std::exp(-b*x)) * secondsPerSmallestTimeStep;
      cur_time = cur_time - x + y;
    }

    //Calculate hand angles
    const float ticks = (secondsPerNewTick == 0.0f) ? -1.0f : std::fmodf(cur_time, secondsPerNewSecond) * 2.0f * PI / secondsPerNewSecond + startOffset;
    const float seconds = std::fmod(cur_time, secondsPerNewMinute) * 2.0f * PI / secondsPerNewMinute + startOffset;
    const float minutes = std::fmod(cur_time, secondsPerNewHour) * 2.0f * PI / secondsPerNewHour + startOffset;
    const float hours = std::fmod(cur_time, secondsPerFullCycle) * 2.0f * PI / secondsPerFullCycle + startOffset;

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
    const float r1 = std::sin(cur_time * 0.017f)*0.5f + 0.5f;
    const float r2 = std::sin(cur_time * 0.011f)*0.5f + 0.5f;
    const float r3 = std::sin(cur_time * 0.003f)*0.5f + 0.5f;
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
    switch (clock_type) {
      case HMST: FractalIter(pt, dir, iters - 1, true, true, true, true); break;
      case HMS: FractalIter(pt, dir, iters - 1, true, true, true, false); break;
      case MST: FractalIter(pt, dir, iters - 1, false, true, true, true); break;
      case HM: FractalIter(pt, dir, iters - 1, true, true, false, false); break;
      case MS: FractalIter(pt, dir, iters - 1, false, true, true, false); break;
      case ST: FractalIter(pt, dir, iters - 1, false, false, true, true); break;
    }

    //Clear the screen
    window.clear(bgnd_color);

    //Draw the fractal branches
    if (draw_branches)
    {
      glEnable(GL_LINE_SMOOTH);
      glLineWidth(2.0f);
      if (!draw_clock)
        window.draw(line_array.data(), line_array.size(), sf::PrimitiveType::Lines);
      else if (clock_type == ClockType::HMST)
        window.draw(line_array.data(), line_array.size() - 8, sf::PrimitiveType::Lines);
      else if (clock_type == ClockType::HMS || clock_type == ClockType::MST)
        window.draw(line_array.data(), line_array.size() - 6, sf::PrimitiveType::Lines);
      else
        window.draw(line_array.data(), line_array.size() - 4, sf::PrimitiveType::Lines);
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
      if (clock_type == ClockType::HM)
      {
        glLineWidth(4.0f);
        window.draw(line_array.data() + line_array.size() - 4, 2, sf::PrimitiveType::Lines);
        glLineWidth(5.0f);
        window.draw(line_array.data() + line_array.size() - 2, 2, sf::PrimitiveType::Lines);
      }
      else if (clock_type == ClockType::HMS)
      {
        glLineWidth(2.0f);
        window.draw(line_array.data() + line_array.size() - 6, 2, sf::PrimitiveType::Lines);
        glLineWidth(4.0f);
        window.draw(line_array.data() + line_array.size() - 4, 2, sf::PrimitiveType::Lines);
        glLineWidth(5.0f);
        window.draw(line_array.data() + line_array.size() - 2, 2, sf::PrimitiveType::Lines);
      }
      else if (clock_type == ClockType::MS)
      {
        glLineWidth(2.0f);
        window.draw(line_array.data() + line_array.size() - 4, 2, sf::PrimitiveType::Lines);
        glLineWidth(4.0f);
        window.draw(line_array.data() + line_array.size() - 2, 2, sf::PrimitiveType::Lines);
      }
      else if (clock_type == ClockType::HMST)
      {
        glLineWidth(1.0f);
        window.draw(line_array.data() + line_array.size() - 8, 2, sf::PrimitiveType::Lines);
        glLineWidth(2.0f);
        window.draw(line_array.data() + line_array.size() - 6, 2, sf::PrimitiveType::Lines);
        glLineWidth(4.0f);
        window.draw(line_array.data() + line_array.size() - 4, 2, sf::PrimitiveType::Lines);
        glLineWidth(5.0f);
        window.draw(line_array.data() + line_array.size() - 2, 2, sf::PrimitiveType::Lines);
      }
      else if (clock_type == ClockType::MST)
      {
        glLineWidth(1.0f);
        window.draw(line_array.data() + line_array.size() - 6, 2, sf::PrimitiveType::Lines);
        glLineWidth(2.0f);
        window.draw(line_array.data() + line_array.size() - 4, 2, sf::PrimitiveType::Lines);
        glLineWidth(4.0f);
        window.draw(line_array.data() + line_array.size() - 2, 2, sf::PrimitiveType::Lines);
      }
      else if (clock_type == ClockType::ST)
      {
        glLineWidth(1.0f);
        window.draw(line_array.data() + line_array.size() - 4, 2, sf::PrimitiveType::Lines);
        glLineWidth(2.0f);
        window.draw(line_array.data() + line_array.size() - 2, 2, sf::PrimitiveType::Lines);
      }
    }

    //Draw UI elements
    if (show_shortcuts)
    {
        clock_type_text.setString(clock_type_name[clock_type]);
        window.draw(clock_type_text);
        time_type_text.setString(time_type_name[time_type]);
        window.draw(time_type_text);
        realtime_text.setString(realtime_name[use_realtime ? 1 : 0]);
        window.draw(realtime_text);
        tick_text.setString(tick_name[use_tick ? 1 : 0]);
        window.draw(tick_text);
        draw_branches_text.setString(draw_branches_name[draw_branches ? 1 : 0]);
        window.draw(draw_branches_text);
        draw_clock_text.setString(draw_clock_name[draw_clock ? 1 : 0]);
        window.draw(draw_clock_text);
        toggle_fullscreen_text.setString(toggle_fullscreen_name[is_fullscreen ? 1 : 0]);
        window.draw(toggle_fullscreen_text);
        show_shortcuts_text.setString(show_shortcuts_name);
        window.draw(show_shortcuts_text);
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
