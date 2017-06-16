#include "uci.h"

void position(std::istringstream & is, State & s)
{
    std::string token, fen;

    is >> token;
    if (token == "fen")
    {
        while (is >> token && token != "moves")
            fen += token + " ";
        s = State(fen);
    }
    std::cout << fen << '\n';
    std::cout << s;
}

void set_option(std::string & name, std::string & value)
{
    if (name == "Hash")
        ttable.resize(std::stoi(value));
    else if (name == "ClearHash")
        ttable.clear();

    return;
}

void uci()
{
    State root(Start_fen);
    std::string command, token;

    while (1)
    {
        std::getline(std::cin, command);
        std::istringstream is(command);
        is >> std::skipws >> token;

        if (token == "quit")
            break;
        else if (token == "isready")
            std::cout << "readyok\n";
        else if (token == "uci")
        {
            std::cout << "id name Clever Girl\n"
                      << "id author Seth Kasmann\n"
                      << "option name Hash type spin default 1 min 1 max 128\n";
            ttable.resize(128);
            std::cout << "uci ok\n";
        }
        else if (token == "setoption")
        {
            std::string name, value;

            is >> token;
            while (is >> token && token != "value")
                name += token;
            while (is >> token)
                value += token;

            set_option(name, value);
        }
        else if (token == "position")
            position(is, root);

    }
}