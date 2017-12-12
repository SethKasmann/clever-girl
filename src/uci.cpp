#include "uci.h"
#include <fstream>

// Check if a move given by the uci is valid.
Move get_uci_move(std::string & token, State & s)
{
    Move m;

    token.erase(std::remove(token.begin(), token.end(), ','),
                    token.end());
    MoveList mlist(s);
    while (mlist.size() > 0)
    {
        m = mlist.pop();
        if (toString(m) == token)
            return m;
        if (mlist.size() == 0)
            return nullMove;
    }
    return nullMove;
}

void go(std::istringstream & is, State & s)
{
    std::string token;
    SearchInfo search_info;
    Move m;

    while (is >> token)
    {
        if (token == "searchmoves")
        {
            while (is >> token)
            {
                m = get_uci_move(token, s);
                if (m != nullMove)
                    search_info.sm.push_back(m);
                else
                {
                    std::cout << "illegal move found: " << token << '\n';
                    return;
                }
            }
        }
        else if (token == "ponder")
            search_info.ponder = true;
        else if (token == "wtime")
            is >> search_info.time[white];
        else if (token == "btime")
            is >> search_info.time[black];
        else if (token == "winc")
            is >> search_info.inc[white];
        else if (token == "binc")
            is >> search_info.inc[black];
        else if (token == "movestogo")
            is >> search_info.moves_to_go;
        else if (token == "depth")
            is >> search_info.depth;
        else if (token == "nodes")
            is >> search_info.max_nodes;
        else if (token == "mate")
            is >> search_info.mate;
        else if (token == "movetime")
            is >> search_info.moveTime;
        else if (token == "infinite")
            search_info.infinite = true;
    }

    search_info.start_time = system_time();
    if (!search_info.moveTime)
        search_info.moveTime = allocate_time(search_info.time[s.getOurColor()], 
                                              history.size() / 2, 
                                              search_info.moves_to_go);
    setup_search(s, search_info);
}

void position(std::istringstream & is, State & s)
{
    std::string token, fen;
    Move m;
    bool start_flag = false;

    s = State(Start_fen);
    history.clear();
    history.push(std::make_pair(nullMove, s.getKey()));

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
        m = get_uci_move(token, s);
        if (m == nullMove)
        {
            std::cout << "illegal move found: " << token << '\n' << s;
            return;
        }
        else
        {
            s.make_t(m);
            history.push(std::make_pair(m, s.getKey()));
        }
    }
    // If start flag is false, initialize board state with fen string.
    if (!start_flag)
        s = State(fen);
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

    std::setvbuf(stdin, NULL, _IONBF, 0);

    while (1)
    {
        std::getline(std::cin, command);
        std::istringstream is(command);
        is >> std::skipws >> token;

        if (token == "quit")
            break;
        else if (token == "isready")
            std::cout << "readyok" << std::endl;
        else if (token == "uci")
        {
            std::cout << "id name Clever Girl" << std::endl
                      << "id author Seth Kasmann" << std::endl
                      << "option name Hash type spin default 1 min 1 max 128" << std::endl;
            std::cout << "uciok" << std::endl;
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