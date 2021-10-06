#include <iostream>
#include <chrono>
#include <random>
#include <cstring>

#include "Movelist.hpp"
#include "Chess_Test.hpp"

class MoveReciever
{
public:
	static inline uint64_t nodes;

	static _ForceInline void Init(Board& brd, uint64_t EPInit) {
		MoveReciever::nodes = 0;
		Movelist::Init(EPInit);
	}

	template<class BoardStatus status>
	static _ForceInline void PerfT0()
	{
		nodes++;
	}
	
	template<class BoardStatus status>
	static _ForceInline void PerfT1(Board& brd)
	{
		nodes += Movelist::count<status>(brd);
	}

	template<class BoardStatus status, int depth>
	static _ForceInline void PerfT(Board& brd)
	{
		Movelist::EnumerateMoves<status, MoveReciever, depth>(brd);
	}

	//This method will see every position //Normal Inline - not forced - heavy recursion
	template<class BoardStatus status, int depth>
	static _ForceInline void RegisterMove(Board& brd)
	{
		if constexpr (depth == 2) {
			PerfT1<status>(brd);
		}
		else PerfT<status, depth - 1>(brd);
	}


#define ENABLEDBG 0
#define ENABLEPRINT 0
#define IFDBG if constexpr (ENABLEDBG) 
#define IFPRN if constexpr (ENABLEPRINT) 

	template<class BoardStatus status, int depth>
	static _Inline void Kingmove(const Board& brd, uint64_t from, uint64_t to)
	{
		Board next = Board::Move<BoardPiece::King, status.WhiteMove>(brd, from, to, to & Enemy<status.WhiteMove>(brd));
		IFPRN std::cout << "Kingmove:\n" << _map(from, to, brd, next) << "\n";
		//IFDBG Board::AssertBoardMove<status.WhiteMove>(brd, next, to & Enemy<status.WhiteMove>(brd));

		RegisterMove<status.KingMove(), depth>(next);
	}

	template<class BoardStatus status, int depth>
	static _Inline void KingCastle(const Board& brd, uint64_t kingswitch, uint64_t rookswitch)
	{
		Board next = Board::MoveCastle<status.WhiteMove>(brd, kingswitch, rookswitch);
		IFPRN std::cout << "KingCastle:\n" << _map(kingswitch, rookswitch, brd, next) << "\n";
		//IFDBG Board::AssertBoardMove<status.WhiteMove>(brd, next, false);
		RegisterMove<status.KingMove(), depth>(next);
	}

	template<class BoardStatus status, int depth>
	static _Inline void PawnCheck(map eking, uint64_t to) {
		constexpr bool white = status.WhiteMove;
		map pl = Pawn_AttackLeft<white>(to & Pawns_NotLeft());
		map pr = Pawn_AttackRight<white>(to & Pawns_NotRight());

		if (eking & (pl | pr)) Movestack::Check_Status[depth - 1] = to;
	}

	template<class BoardStatus status, int depth>
	static _Inline void KnightCheck(map eking, uint64_t to) {
		constexpr bool white = status.WhiteMove;

		if (Lookup::Knight(SquareOf(eking)) & to) Movestack::Check_Status[depth - 1] = to;
	}
	

	template<class BoardStatus status, int depth>
	static _Inline void Pawnmove(const Board& brd, uint64_t from, uint64_t to)
	{
		Board next = Board::Move<BoardPiece::Pawn, status.WhiteMove, false>(brd, from, to);
		IFPRN std::cout << "Pawnmove:\n" << _map(from, to, brd, next) << "\n";
		//IFDBG Board::AssertBoardMove<status.WhiteMove>(brd, next, to & Enemy<status.WhiteMove>(brd));
		PawnCheck<status, depth>(EnemyKing<status.WhiteMove>(brd), to);
		RegisterMove<status.SilentMove(), depth>(next);
		Movestack::Check_Status[depth - 1] = 0xffffffffffffffffull;
	}

	template<class BoardStatus status, int depth>
	static _Inline void Pawnatk(const Board& brd, uint64_t from, uint64_t to)
	{
		Board next = Board::Move<BoardPiece::Pawn, status.WhiteMove, true>(brd, from, to);
		IFPRN std::cout << "Pawntake:\n" << _map(from, to, brd, next) << "\n";
		//IFDBG Board::AssertBoardMove<status.WhiteMove>(brd, next, to & Enemy<status.WhiteMove>(brd));
		PawnCheck<status, depth>(EnemyKing<status.WhiteMove>(brd), to);
		RegisterMove<status.SilentMove(), depth>(next);
		Movestack::Check_Status[depth - 1] = 0xffffffffffffffffull;
	}

	template<class BoardStatus status, int depth>
	static _Inline void PawnEnpassantTake(const Board& brd, uint64_t from, uint64_t enemy, uint64_t to)
	{
		Board next = Board::MoveEP<status.WhiteMove>(brd, from, enemy, to);
		IFPRN std::cout << "PawnEnpassantTake:\n" << _map(from | enemy, to, brd, next) << "\n";
		//IFDBG Board::AssertBoardMove<status.WhiteMove>(brd, next, true);
		PawnCheck<status, depth>(EnemyKing<status.WhiteMove>(brd), to);
		RegisterMove<status.SilentMove(), depth>(next);
		Movestack::Check_Status[depth - 1] = 0xffffffffffffffffull;
	}

	template<class BoardStatus status, int depth>
	static _Inline void Pawnpush(const Board& brd, uint64_t from, uint64_t to)
	{
		Board next = Board::Move <BoardPiece::Pawn, status.WhiteMove, false>(brd, from, to);
		IFPRN std::cout << "Pawnpush:\n" << _map(from, to, brd, next) << "\n";
		//IFDBG Board::AssertBoardMove<status.WhiteMove>(brd, next, to & Enemy<status.WhiteMove>(brd));

		Movelist::EnPassantTarget = to;
		PawnCheck<status, depth>(EnemyKing<status.WhiteMove>(brd), to);
		RegisterMove<status.PawnPush(), depth>(next);
		Movestack::Check_Status[depth - 1] = 0xffffffffffffffffull;
	}

	template<class BoardStatus status, int depth>
	static _Inline void Pawnpromote(const Board& brd, uint64_t from, uint64_t to)
	{
		Board next1 = Board::MovePromote<BoardPiece::Queen, status.WhiteMove>(brd, from, to);
		IFPRN std::cout << "Pawnpromote:\n" << _map(from, to, brd, next1) << "\n";
		//IFDBG Board::AssertBoardMove<status.WhiteMove>(brd, next1, to & Enemy<status.WhiteMove>(brd));
		RegisterMove<status.SilentMove(), depth>(next1);

		Board next2 = Board::MovePromote<BoardPiece::Knight, status.WhiteMove>(brd, from, to);
		KnightCheck<status, depth>(EnemyKing<status.WhiteMove>(brd), to);
		RegisterMove<status.SilentMove(), depth>(next2);
		Movestack::Check_Status[depth - 1] = 0xffffffffffffffffull;

		Board next3 = Board::MovePromote<BoardPiece::Bishop, status.WhiteMove>(brd, from, to);
		RegisterMove<status.SilentMove(), depth>(next3);
		Board next4 = Board::MovePromote<BoardPiece::Rook, status.WhiteMove>(brd, from, to);
		RegisterMove<status.SilentMove(), depth>(next4);
	}

	template<class BoardStatus status, int depth>
	static _Inline void Knightmove(const Board& brd, uint64_t from, uint64_t to)
	{
		Board next = Board::Move <BoardPiece::Knight, status.WhiteMove>(brd, from, to, to & Enemy<status.WhiteMove>(brd));
		IFPRN std::cout << "Knightmove:\n" << _map(from, to, brd, next) << "\n";
		//IFDBG Board::AssertBoardMove<status.WhiteMove>(brd, next, to & Enemy<status.WhiteMove>(brd));
		KnightCheck<status, depth>(EnemyKing<status.WhiteMove>(brd), to);
		RegisterMove<status.SilentMove(), depth>(next);
		Movestack::Check_Status[depth - 1] = 0xffffffffffffffffull;
	}

	template<class BoardStatus status, int depth>
	static _Inline void Bishopmove(const Board& brd, uint64_t from, uint64_t to)
	{
		Board next = Board::Move <BoardPiece::Bishop, status.WhiteMove>(brd, from, to, to & Enemy<status.WhiteMove>(brd));
		IFPRN std::cout << "Bishopmove:\n" << _map(from, to, brd, next) << "\n";
		//IFDBG Board::AssertBoardMove<status.WhiteMove>(brd, next, to & Enemy<status.WhiteMove>(brd));
		RegisterMove<status.SilentMove(), depth>(next);
	}

	template<class BoardStatus status, int depth>
	static _Inline void Rookmove(const Board& brd, uint64_t from, uint64_t to)
	{
		Board next = Board::Move<BoardPiece::Rook, status.WhiteMove>(brd, from, to, to & Enemy<status.WhiteMove>(brd));
		IFPRN std::cout << "Rookmove:\n" << _map(from, to, brd, next) << "\n";
		//IFDBG Board::AssertBoardMove<status.WhiteMove>(brd, next, to & Enemy<status.WhiteMove>(brd));
		if constexpr (status.CanCastle()) {
			if (status.IsLeftRook(from)) RegisterMove<status.RookMove_Left(), depth>(next);
			else if (status.IsRightRook(from)) RegisterMove<status.RookMove_Right(), depth>(next);
			else RegisterMove<status.SilentMove(), depth>(next);
		}
		else RegisterMove<status.SilentMove(), depth>(next);
	}

	template<class BoardStatus status, int depth>
	static _Inline void Queenmove(const Board& brd, uint64_t from, uint64_t to)
	{
		Board next = Board::Move<BoardPiece::Queen, status.WhiteMove>(brd, from, to, to & Enemy<status.WhiteMove>(brd));
		IFPRN std::cout << "Queenmove:\n" << _map(from, to, brd, next) << "\n";
		//IFDBG Board::AssertBoardMove<status.WhiteMove>(brd, next, to & Enemy<status.WhiteMove>(brd));
		RegisterMove<status.SilentMove(), depth>(next);
	}
};


template<class BoardStatus status>
static void PerfT(std::string_view def, Board& brd, int depth)
{
	MoveReciever::Init(brd, FEN::FenEnpassant(def));

	//Seemap see;
	//Movegen::InitBoard<status>(see, brd.UnpackAll());

	/// <summary>
	/// Go into recursion on depth 2 - entry point for perft
	/// </summary>
	switch (depth)
	{
		case 0: Movelist::InitStack<status, 0>(brd); MoveReciever::PerfT0<status>(); return;
		case 1: Movelist::InitStack<status, 1>(brd); MoveReciever::PerfT1<status>(brd); return; //Keep this as T1
		case 2: Movelist::InitStack<status, 2>(brd); MoveReciever::PerfT<status, 2>(brd); return;
		case 3: Movelist::InitStack<status, 3>(brd); MoveReciever::PerfT<status, 3>(brd);  return;
		case 4: Movelist::InitStack<status, 4>(brd); MoveReciever::PerfT<status, 4>(brd);  return;
		case 5: Movelist::InitStack<status, 5>(brd); MoveReciever::PerfT<status, 5>(brd);  return;
		case 6: Movelist::InitStack<status, 6>(brd); MoveReciever::PerfT<status, 6>(brd);  return;
		case 7: Movelist::InitStack<status, 7>(brd); MoveReciever::PerfT<status, 7>(brd);  return;
		case 8: Movelist::InitStack<status, 8>(brd); MoveReciever::PerfT<status, 8>(brd);  return;
		case 9: Movelist::InitStack<status, 9>(brd); MoveReciever::PerfT<status, 9>(brd);  return;
		case 10: Movelist::InitStack<status, 10>(brd); MoveReciever::PerfT<status, 10>(brd); return;
		case 11: Movelist::InitStack<status, 11>(brd); MoveReciever::PerfT<status, 11>(brd); return;
		case 12: Movelist::InitStack<status, 12>(brd); MoveReciever::PerfT<status, 12>(brd); return;
		case 13: Movelist::InitStack<status, 13>(brd); MoveReciever::PerfT<status, 13>(brd); return;
		case 14: Movelist::InitStack<status, 14>(brd); MoveReciever::PerfT<status, 14>(brd); return;
		case 15: Movelist::InitStack<status, 15>(brd); MoveReciever::PerfT<status, 15>(brd); return;
		case 16: Movelist::InitStack<status, 16>(brd); MoveReciever::PerfT<status, 16>(brd); return;
		case 17: Movelist::InitStack<status, 17>(brd); MoveReciever::PerfT<status, 17>(brd); return;
		case 18: Movelist::InitStack<status, 18>(brd); MoveReciever::PerfT<status, 18>(brd); return;
		default:
			std::cout << "Depth not impl yet" << std::endl;
			return;
	}
}
PositionToTemplate(PerfT);

void Chess_Test() {
	for (auto pos : Test::Positions)
	{
		auto v = Test::GetElements(pos, ';');
		std::string fen = v[0];

		std::cout << fen << "\n";
		int to = v.size();
		for (int i = 1; i < to; i++) {
			auto perftvals = Test::GetElements(v[i], ' ');
			uint64_t expected = static_cast<uint64_t>(std::strtol(perftvals[1].c_str(), NULL, 10));
			_PerfT(fen, i);
			uint64_t result = MoveReciever::nodes;
			std::string status = expected == result ? "OK" : "ERROR";
			if (expected == result)  std::cout << "   " << i << ": " << result << " " << status << "\n";
			else  std::cout << "xxx -> " << i << ": " << result <<" vs " << expected << " " << status << "\n";
			
		}
	}
}

const auto _keep0 = _map(0);
const auto _keep1 = _map(0,0);
const auto _keep2 = _map(0,0,0);
const auto _keep4 = _map(0, 0, Board::Default(), Board::Default());

int main(int argc, char** argv)
{
	std::vector<std::string> args(argv, argv + argc);
	if (args.size() >= 4) {
		std::string_view def(argv[1]);
		uint64_t depth = static_cast<uint64_t>(std::strtol(args[2].c_str(), NULL, 10));
		uint64_t exptected = static_cast<uint64_t>(std::strtol(args[3].c_str(), NULL, 10));

		_PerfT(def, static_cast<int>(depth));
		if (exptected == MoveReciever::nodes) return 0;
		return 9;
	}
	if (args.size() == 3) {
		std::string_view def(argv[1]);
		uint64_t depth = static_cast<uint64_t>(std::strtol(args[2].c_str(), NULL, 10));
		if (depth > 12) { std::cout << "Max depth limited to 12 for now!\n"; return 0; }
		std::cout << "Depth: " << depth << " - " << def << "\n";
		for (int i = 1; i <= depth; i++)
		{
			auto start = std::chrono::steady_clock::now();
			_PerfT(def, i);
			auto end = std::chrono::steady_clock::now();
			long long delta = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
			std::cout << "Perft " << i << ": " << MoveReciever::nodes << " " << delta / 1000 << "ms " << MoveReciever::nodes * 1.0 / delta << " MNodes/s\n";
		}

		std::cout << "\nPress any key to exit..."; getchar();
		return 0;
	}

	//Chess_Test();
	//return 0;
	
	std::random_device rd;
	std::mt19937_64 eng(rd());
	std::uniform_int_distribution<unsigned long long> distr;

	//std::string_view dbg = "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8"

	std::string_view def = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
	std::string_view kiwi = "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1";
	//std::string_view pintest = "6Q1/8/4k3/8/4r3/1K6/4R3/8 b - - 0 1";
	//std::string_view def = "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq -";
	std::string_view midgame = "r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10";
	std::string_view endgame = "5nk1/pp3pp1/2p4p/q7/2PPB2P/P5P1/1P5K/3Q4 w - - 1 28";
	//55.8

	auto ts = std::chrono::steady_clock::now();
	for (int i = 1; i <= 7; i++)
	{
		auto start = std::chrono::steady_clock::now();
		_PerfT(def, i);
		auto end = std::chrono::steady_clock::now();
		long long delta = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
		std::cout << "Perft Start " <<i<< ": "<< MoveReciever::nodes << " " << delta / 1000 <<"ms " << MoveReciever::nodes * 1.0 / delta << " MNodes/s\n";
	}
	if (MoveReciever::nodes == 3195901860ull) std::cout << "OK\n\n";
	else std::cout << "ERROR!\n\n";

	for (int i = 1; i <= 6; i++)
	{
		auto start = std::chrono::steady_clock::now();
		_PerfT(kiwi, i);
		auto end = std::chrono::steady_clock::now();
		long long delta = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
		std::cout << "Perft Kiwi " << i << ": " << MoveReciever::nodes << " " << delta / 1000 << "ms " << MoveReciever::nodes * 1.0 / delta << " MNodes/s\n";
	}
	if (MoveReciever::nodes == 8031647685ull) std::cout << "OK\n\n";
	else std::cout << "ERROR!\n\n";

	for (int i = 1; i <= 6; i++)
	{
		auto start = std::chrono::steady_clock::now();
		_PerfT(midgame, i);
		auto end = std::chrono::steady_clock::now();
		long long delta = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
		std::cout << "Perft Midgame " << i << ": " << MoveReciever::nodes << " " << delta / 1000 << "ms " << MoveReciever::nodes * 1.0 / delta << " MNodes/s\n";
	}
	if (MoveReciever::nodes == 6923051137ull) std::cout << "OK\n\n";
	else std::cout << "ERROR!\n\n";

	for (int i = 1; i <= 6; i++)
	{
		auto start = std::chrono::steady_clock::now();
		_PerfT(endgame, i);
		auto end = std::chrono::steady_clock::now();
		long long delta = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
		std::cout << "Perft Endgame " << i << ": " << MoveReciever::nodes << " " << delta / 1000 << "ms " << MoveReciever::nodes * 1.0 / delta << " MNodes/s\n";
	}
	if (MoveReciever::nodes == 849167880ull) std::cout << "OK\n\n";
	else std::cout << "ERROR!\n\n";

	//Total nodes
	MoveReciever::nodes = (3195901860ull + 8031647685ull + 6923051137ull + 849167880ull);
	auto te = std::chrono::steady_clock::now();
	long long total = std::chrono::duration_cast<std::chrono::microseconds>(te - ts).count();
	std::cout << "Perft aggregate: " << MoveReciever::nodes
		<< " " << total / 1000 << "ms " << MoveReciever::nodes * 1.0 / total << " MNodes/s\n";

	std::cout << "\nPress any key to exit..."; getchar();
}
