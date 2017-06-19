#include "uci.h"

void go(std::istringstream & is, State & s)
{
    std::cout << to_string(search(s)) << '\n';
    std::cout << s;
}

void position(std::istringstream & is, State & s)
{
    std::string token, fen;
    MoveList mlist;
    Move m;
    bool start_flag = false;

    s = State(Start_fen);
    glist.clear();
    glist.push(No_move, s.key);

    is >> token;
    if (token == "fen")
    {
        while (is >> token && token != "moves")
            fen += token + " ";
    }
    else if (token == "startpos")
    {
        start_flag = true;
        is >> token;
    }
    else
    {
        std::cout << "unknown command\n";
        return;
    }

    while (is >> token)
    {
        token.erase(std::remove(token.begin(), token.end(), ','), 
                    token.end());
        push_moves(s, &mlist);
        while (mlist.size() > 0)
        {
            m = mlist.pop();
            if (to_string(m) == token)
            {
                // Only make moves if the start flag is true.
                s.make(m);
                glist.push(m, s.key);
                break;
            }
            if (mlist.size() == 0)
            {
                std::cout << "illegal move found: " << token << '\n';
                return;
            }
        }
        mlist.clear();
    }
    // If start flag is false, initialize board state with fen string.
    if (!start_flag)
        s = State(fen);
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
        else if (token == "go")
            go(is, root);

    }
}