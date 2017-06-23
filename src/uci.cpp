#include "uci.h"

// Check if a move given by the uci is valid.
Move get_uci_move(std::string & token, State & s)
{
    MoveList mlist;
    Move m;

    token.erase(std::remove(token.begin(), token.end(), ','),
                    token.end());
    push_moves(s, &mlist);
    while (mlist.size() > 0)
    {
        m = mlist.pop();
        if (to_string(m) == token)
            return m;
        if (mlist.size() == 0)
            return No_move;
    }
    return No_move;
}

void go(std::istringstream & is, State & s)
{
    std::string token;
    SearchInfo search_info;
    Move m;

    is >> token;

    if (token == "searchmoves")
    {
        while (is >> token)
        {
            m = get_uci_move(token, s);
            if (m != No_move)
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
        is >> search_info.nodes;
    else if (token == "mate")
        is >> search_info.mate;
    else if (token == "movetime")
        is >> search_info.move_time;
    else if (token == "infinite")
        search_info.infinite = true;

    setup_search(s, search_info);
}

void position(std::istringstream & is, State & s)
{
    std::string token, fen;
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
        m = get_uci_move(token, s);
        if (m == No_move)
        {
            std::cout << "illegal move found: " << token << '\n';
            return;
        }
        else
        {
            s.make(m);
            glist.push(m, s.key);
        }
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