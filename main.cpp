#include "ohcli.h"
#include <iostream>
int main(int argc, char **argv)
{
  ohcli::CLI cli;
  double range = 0;
  int oneof = 0;
  bool option = false;
  std::string regex;
  
  cli.add_value("s", regex, ohcli::email());
  //or cli.add_value("s", regex, ohcli::regex("^\\w+([-+.]\\w+)*@\\w+([-.]\\w+)*\\.\\w+([-.]\\w+)*$"));
  cli.add_value("r", range, ohcli::range<double>(0.0, 1.0));
  cli.add_value("f", "oneof", oneof, ohcli::oneof({1, 3, 5}));
  //or cli.add_value("f", "oneof", oneof, ohcli::oneof(<some container, e.g. std::vector><int>{1,3,5}));
  cli.add_option("o", "option", option);
  cli.add_cmd("p", "print", [](ohcli::CmdArg &args)
  {
    std::cout << "print: ";
    for (auto &r: args)
      std::cout << "\"" << r << "\" ";
    std::cout << std::endl;
  });
  cli.parse(argc, argv);
  cli.run();
  return 0;
}
