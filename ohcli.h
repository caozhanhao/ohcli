//   Copyright 2022 ohcli - caozhanhao
//
//   Licensed under the Apache License, Version 2.0 (the "License");
//   you may not use this file except in compliance with the License.
//   You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
//   Unless required by applicable law or agreed to in writing, software
//   distributed under the License is distributed on an "AS IS" BASIS,
//   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//   See the License for the specific language governing permissions and
//   limitations under the License.
#ifndef OHCLI_OPTION_H
#define OHCLI_OPTION_H
#include "err.h"
#include <vector>
#include <functional>
#include <regex>
#include <map>
#include <string>
#include <algorithm>
#include <cstring>
#include <iostream>

namespace ohcli
{
  using CmdArg = std::vector<std::string>;
  using Cmd = std::function<void(CmdArg &)>;
  template<typename T>
  using Restrictor = std::function<bool(T &)>;
  
  template<typename T>
  Restrictor<T> range(T a, T b)
  {
    return [a, b](T &v) -> bool { return v >= a && v < b; };
  }
  
  template<typename T>
  Restrictor<typename T::value_type> oneof(const T &s)
  {
    return [s](typename T::value_type &v) -> bool
    {
      return std::find(std::cbegin(s), std::cend(s), v) != std::cend(s);
    };
  }
  
  template<typename T>
  Restrictor<T> oneof(const std::initializer_list<T> &s)
  {
    return [s](T &v) -> bool { return std::find(std::cbegin(s), std::cend(s), v) != std::cend(s); };
  }
  
  template<typename T>
  Restrictor<T> default_restrictor()
  {
    return [](T &) -> bool { return true; };
  }
  
  Restrictor<std::string> regex(const std::string &pattern)
  {
    return [pattern](std::string &v) -> bool
    {
      return std::regex_match(v, std::regex{pattern});
    };
  }
  
  Restrictor<std::string> email()
  {
    return regex("^\\w+([-+.]\\w+)*@\\w+([-.]\\w+)*\\.\\w+([-.]\\w+)*$");
  }
  
  namespace utils
  {
    class Error : public std::runtime_error
    {
    public:
      Error(std::string details)
          : runtime_error(details) {}
    };
    
    void fatal(const std::string &str)
    {
      throw std::logic_error("\033[31mFATAL: \033[0m" + str);
    }
    
    void error(const std::string &str)
    {
      throw std::runtime_error("\033[31mERROR: \033[0m" + str);
    }
    
    void warn(const std::string &str)
    {
      std::cout << "\033[33mWARNING: \033[0m" << str << std::endl;
    }
    
    template<typename T>
    T str_to(const std::string &s);
    
    template<>
    std::string str_to(const std::string &s)
    {
      return s;
    }
    
    template<>
    double str_to(const std::string &s)
    {
      try
      {
        return std::stod(s);
      }
      catch (...)
      {
        error("Unexpected conversion of '" + s + "' to double.");
      }
      return 0;
    }
    
    template<>
    int str_to(const std::string &s)
    {
      try
      {
        return std::stoi(s);
      }
      catch (...)
      {
        error("Unexpected conversion of '" + s + "' to int.");
      }
      return 0;
    }
    
    template<>
    long str_to(const std::string &s)
    {
      try
      {
        return std::stol(s);
      }
      catch (...)
      {
        error("Unexpected conversion of '" + s + "' to long.");
      }
      return 0;
    }
    
    template<>
    long long str_to(const std::string &s)
    {
      try
      {
        return std::stoll(s);
      }
      catch (...)
      {
        error("Unexpected conversion of '" + s + "' to long long.");
      }
      return 0;
    }
    
    template<>
    long double str_to(const std::string &s)
    {
      try
      {
        return std::stold(s);
      }
      catch (...)
      {
        error("Unexpected conversion of '" + s + "' to long double.");
      }
      return 0;
    }
    
    template<>
    unsigned long str_to(const std::string &s)
    {
      try
      {
        return std::stoul(s);
      }
      catch (...)
      {
        error("Unexpected conversion of '" + s + "' to unsigned long.");
      }
      return 0;
    }
    
    template<>
    unsigned long long str_to(const std::string &s)
    {
      try
      {
        return std::stoull(s);
      }
      catch (...)
      {
        error("Unexpected conversion of '" + s + "' to unsigned long long.");
      }
      return 0;
    }
    
    template<>
    bool str_to(const std::string &s)
    {
      if (s == "true" || s == "True" || s == "TRUE")
        return true;
      else if (s == "false" || s == "False" || s == "FALSE")
        return false;
      else
        error("Unexpected conversion of '" + s + "' to boolean.");
      return true;
    }
  }
  class CLI
  {
  private:
    class Token
    {
    public:
      std::string cmd;
      std::vector<std::string> args;
    public:
      explicit Token(std::string arg, const std::initializer_list<std::string> &values_ = {})
          : cmd(std::move(arg)), args(values_) {}
      
      void add(const std::string &v) { args.emplace_back(v); }
    };
    
    class Packed
    {
    private:
      std::function<void()> packed;
    public:
      int priority;
    public:
      Packed(std::function<void()> packed, int priority_)
          : packed(std::move(packed)), priority(priority_) {}
      
      void operator()()
      {
        packed();
      }
    };
    
    class Callback
    {
    private:
      std::string name;
      Cmd func;
      int expected_args;
      int priority;
    public:
      Callback(std::string name_, Cmd func, int expected_args_, int priority_)
          : name(name_), func(std::move(func)), expected_args(expected_args_), priority(priority_) {}
      
      Callback() : func([](CmdArg) {}), expected_args(-1), priority(-1) {}
      
      Packed pack(CmdArg arg)
      {
        if (expected_args != -1)
        {
          if (arg.size() < expected_args)
          {
            utils::error(name + ": " + "Too few arguments (" + std::to_string(arg.size()) + "), expects "
                         + std::to_string(expected_args));
          }
          if (arg.size() > expected_args)
          {
            utils::warn(name + ": " + "Expected " + std::to_string(expected_args) + " arguments, but "
                        + std::to_string(arg.size()) + " was given.");
          }
        }
        return {std::bind(func, arg), priority};
      }
    };
  
  private:
    std::vector<Token> tokens;
    std::map<const std::string, const std::string> alias;
    std::map<const std::string, Callback> funcs;
    std::vector<Packed> tasks;
    bool parsed;
  public:
    CLI()
        : parsed(false) {}
    
    CLI &add_cmd(const std::string &cmd, const Cmd &func, int expected_args = -1, int priority = -1)
    {
      if (parsed)
        utils::fatal("Can not add_cmd() after parse().");
      if (funcs.find(cmd) != funcs.end())
        utils::fatal("Duplicate names are prohibited.('" + cmd + "').");
      funcs.insert(std::make_pair(cmd, Callback(cmd, func, expected_args, priority)));
      return *this;
    }
    
    CLI &
    add_cmd(const std::string &cmd, const std::string &alia, const Cmd &func, int expected_args = -1, int priority = -1)
    {
      if (parsed)
        utils::fatal("Can not add_cmd() after parse().");
      if (funcs.find(cmd) != funcs.end())
        utils::fatal("Duplicate names are prohibited.('" + cmd + "').");
      if (alias.find(alia) != alias.end())
        utils::fatal("Duplicate names are prohibited.('" + alia + "').");
      funcs.insert(std::make_pair(cmd, Callback(cmd, func, expected_args, priority)));
      alias.insert(std::make_pair(alia, cmd));
      return *this;
    }
    
    template<typename T>
    CLI &add_value(const std::string &name, T &value, Restrictor<T> restrictor = default_restrictor<T>())
    {
      if (parsed)
        utils::fatal("Can not add_value() after parse().");
      add_cmd(name,
              [&value, restrictor](CmdArg &args)
              {
                T temp = utils::str_to<T>(args[0]);
                if (!restrictor(temp))
                  utils::error("Invaild value '" + args[0] + "'");
                value = temp;
              },
              1);
      return *this;
    }
    
    template<typename T>
    CLI &add_value(const std::string &name, const std::string &alia, T &value,
                   Restrictor<T> restrictor = default_restrictor<T>())
    {
      if (parsed)
        utils::fatal("Can not add_value() after parse().");
      add_cmd(name, alia,
              [&value, restrictor](CmdArg &args)
              {
                T temp = utils::str_to<T>(args[0]);
                if (!restrictor(temp))
                  utils::error("Invaild value '" + args[0] + "'");
                value = temp;
              },
              1);
      return *this;
    }
    
    CLI &add_option(const std::string &name, bool &option)
    {
      if (parsed)
        utils::fatal("Can not add_value() after parse().");
      add_cmd(name,
              [&option](CmdArg &args)
              {
                option = true;
              }, 0);
      return *this;
    }
    
    CLI &add_option(const std::string &name, const std::string &alia, bool &option)
    {
      if (parsed)
        utils::fatal("Can not add_value() after parse().");
      add_cmd(name, alia,
              [&option](CmdArg &args)
              {
                option = true;
              }, 0);
      return *this;
    }
    
    CLI &run()
    {
      if (!parsed)
        utils::fatal("Option has not parsed.");
      for (auto &r: tasks)
        r();
      return *this;
    }
    
    CLI &parse(int argc, char **argv)
    {
      tokens.emplace_back(Token(argv[0]));
      for (int i = 1; i < argc; i++)
      {
        if (argv[i][0] == '-' && strlen(argv[i]) != 1)
        {
          if (argv[i][1] == '-' && strlen(argv[i]) != 2)
            tokens.emplace_back(Token(std::string(std::string(argv[i]), 2)));//--x
          else
            tokens.emplace_back(Token(std::string(std::string(argv[i]), 1)));//-x
        }
        else
          tokens.back().add(std::string(argv[i]));//-
      }
      funcs.insert(std::make_pair(argv[0], Callback()));
      parse_multi();
      for (auto &r: tokens)
      {
        if (funcs.find(r.cmd) != funcs.cend())
        {
          tasks.emplace_back(funcs[r.cmd].pack(r.args));
          continue;
        }
        if (alias.find(r.cmd) != alias.cend())
        {
          tasks.emplace_back(funcs[alias[r.cmd]].pack(r.args));
          continue;
        }
        utils::warn("Unrecognized option '" + r.cmd + "'.");
        for (auto &a: r.args)
          utils::warn("Discarded arguments '" + a + "'");
      }
      std::sort(tasks.begin(), tasks.end(),
                [](const Packed &p1, const Packed &p2) { return p1.priority > p2.priority; });
      
      parsed = true;
      return *this;
    }
  
  private:
    void parse_multi()
    {
      for (auto it = tokens.cbegin(); it < tokens.cend(); it++)
      {
        if (is_multi(it->cmd))
        {
          for (std::size_t i = 0; i < it->cmd.size(); i++)
            it = 1 + tokens.insert(it, Token(std::string(1, it->cmd[i])));
          for (auto &r: it->args)
            utils::warn("Discarded arguments '" + r + "'");
          tokens.erase(it);
        }
      }
    }
    
    bool is_multi(const std::string &str)
    {
      if (funcs.find(str) != funcs.cend() || alias.find(str) != alias.cend())
        return false;
      return std::all_of(str.begin(), str.end(),
                         [this](char r)
                         {
                           return !(funcs.find(std::string(1, r)) == funcs.cend() &&
                                    alias.find(std::string(1, r)) == alias.cend());
                         }
      );
    }
  };
}
#endif