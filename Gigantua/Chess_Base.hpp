#pragma once

#include <iostream>
#include <string_view>
#include <assert.h>
#include "Movemap.hpp"

#define m_assert(expr, msg) assert(( (void)(msg), (expr) ))

//Lookup_Switch - constexpr enabled - uses ifchain to calculate seen squares
//Lookup_Hash - constexpr enabled - uses multiply lookup
//Lookup_Pext - fastest on hardware pext cpus ryzen 5000+ / intel - uses pext lookup

//This is the most important definition for performance!
namespace Lookup = Chess_Lookup::Lookup_Pext;
typedef uint64_t Bit;    //Single set bit - 1 << [0..63]
typedef uint64_t Square; //Number between [0, 63]
typedef uint64_t map;    //Bitboard - any number. Print with _map()

/*
_Compiletime Pos PopBit(uint64_t& val)
{
    uint64_t lsb = (val & -val);
    val &= (val - 1);
    return lsb;
}
*/

#define SquareOf(X) _tzcnt_u64(X)
#define Bitloop(X) for(;X; X = _blsr_u64(X))

//Bitloop_Inline is 50% slower than bitloop
/* 
#define Bitloop_Inline(X, Y) {\
map sq; \
switch (Bitcount(X)) {\
    case 10: sq = _tzcnt_u64(X); X = _blsr_u64(X); Y; \
    case 9:  sq = _tzcnt_u64(X); X = _blsr_u64(X); Y; \
    case 8:  sq = _tzcnt_u64(X); X = _blsr_u64(X); Y; \
    case 7:  sq = _tzcnt_u64(X); X = _blsr_u64(X); Y; \
    case 6:  sq = _tzcnt_u64(X); X = _blsr_u64(X); Y; \
    case 5:  sq = _tzcnt_u64(X); X = _blsr_u64(X); Y; \
    case 4:  sq = _tzcnt_u64(X); X = _blsr_u64(X); Y; \
    case 3:  sq = _tzcnt_u64(X); X = _blsr_u64(X); Y; \
    case 2:  sq = _tzcnt_u64(X); X = _blsr_u64(X); Y; \
    case 1:  sq = _tzcnt_u64(X); Y; break; \
}}
*/

_Inline static Bit PopBit(uint64_t& val)
{
    uint64_t lsb = _blsi_u64(val);
    //val = _blsr_u64(val); - 3% slower? Todo: Extensive benchmarking
    val ^= lsb;
    return lsb;
}


//Constexpr class as template parameter
class BoardStatus {
    static constexpr uint64_t WNotOccupiedL = 0b01110000ull;
    static constexpr uint64_t WNotAttackedL = 0b00111000ull;

    static constexpr uint64_t WNotOccupiedR = 0b00000110ull;
    static constexpr uint64_t WNotAttackedR = 0b00001110ull;

    static constexpr uint64_t BNotOccupiedL = 0b01110000ull << 56ull;
    static constexpr uint64_t BNotAttackedL = 0b00111000ull << 56ull;

    static constexpr uint64_t BNotOccupiedR = 0b00000110ull << 56ull;
    static constexpr uint64_t BNotAttackedR = 0b00001110ull << 56ull;

    static constexpr uint64_t WRookL_Change = 0b11111000ull;
    static constexpr uint64_t BRookL_Change = 0b11111000ull << 56ull;
    static constexpr uint64_t WRookR_Change = 0b00001111ull;
    static constexpr uint64_t BRookR_Change = 0b00001111ull << 56ull;

    static constexpr uint64_t WRookL = 0b10000000ull;
    static constexpr uint64_t BRookL = 0b10000000ull << 56ull;
    static constexpr uint64_t WRookR = 0b00000001ull;
    static constexpr uint64_t BRookR = 0b00000001ull << 56ull;

public:
    const bool WhiteMove;
    const bool HasEPPawn;

    const bool WCastleL;
    const bool WCastleR;

    const bool BCastleL;
    const bool BCastleR;


    constexpr BoardStatus(bool white, bool ep, bool wcast_left, bool wcast_right, bool bcast_left, bool bcast_right) :
        WhiteMove(white), HasEPPawn(ep), WCastleL(wcast_left), WCastleR(wcast_right), BCastleL(bcast_left), BCastleR(bcast_right)
    {

    }

    constexpr BoardStatus(int pat) :
       WhiteMove((pat & 0b100000) != 0), HasEPPawn((pat & 0b010000) != 0), 
        WCastleL((pat & 0b001000) != 0), WCastleR((pat & 0b000100) != 0), 
        BCastleL((pat & 0b000010) != 0), BCastleR((pat & 0b000001) != 0)
    {

    }

    constexpr bool CanCastle() const {
        if (WhiteMove) return WCastleL | WCastleR;
        else return BCastleL | BCastleR;
    }

    constexpr bool CanCastleLeft() const {
        if (WhiteMove) return WCastleL;
        else return BCastleL;
    }

    constexpr bool CanCastleRight() const {
        if (WhiteMove) return WCastleR;
        else return BCastleR;
    }

    constexpr uint64_t Castle_RookswitchR() const {
        if (WhiteMove) return 0b00000101ull;
        else return 0b00000101ull << 56;
    }
    constexpr uint64_t Castle_RookswitchL() const {
        if (WhiteMove) return 0b10010000ull;
        else return 0b10010000ull << 56;
    }

    //https://lichess.org/analysis/r3k2r/p1pppppp/8/1R6/1r6/8/P1PPPPPP/R3K2R_w_KQkq_-_0_1

    //Returns if Castling Left is an option
    _Inline constexpr bool CanCastleLeft(map attacked, map occupied, map rook) const {
        if (WhiteMove && WCastleL)
        {
            if (occupied & WNotOccupiedL) return false;
            if (attacked & WNotAttackedL) return false;
            if (rook & WRookL) return true; 
            return false;
        }
        else if (BCastleL) {
            if (occupied & BNotOccupiedL) return false;
            if (attacked & BNotAttackedL) return false;
            if (rook & BRookL) return true;
            return false;
        }
        return false;
    }

    _Inline constexpr bool CanCastleRight(map attacked, map occupied, map rook) const {
        if (WhiteMove && WCastleR)
        {
            if (occupied & WNotOccupiedR) return false;
            if (attacked & WNotAttackedR) return false;
            if (rook & WRookR) return true;
            return false;
        }
        else if (BCastleR) {
            if (occupied & BNotOccupiedR) return false;
            if (attacked & BNotAttackedR) return false; 
            if (rook & BRookR) return true;
            return false;
        }
        return false;
    }

    constexpr bool IsLeftRook(Bit rook) const {
        if (WhiteMove) return WRookL == rook;
        else return BRookL == rook;
    }
    constexpr bool IsRightRook(Bit rook) const {
        if (WhiteMove) return WRookR == rook;
        else return BRookR == rook;
    }


    constexpr BoardStatus PawnPush() const {
        return BoardStatus(!WhiteMove, true, WCastleL, WCastleR, BCastleL, BCastleR);
    }

    //Moving the king
    constexpr BoardStatus KingMove() const {
        if (WhiteMove) {
            return BoardStatus(!WhiteMove, false, false, false, BCastleL, BCastleR);
        }
        else {
            return BoardStatus(!WhiteMove, false, WCastleL, WCastleR, false, false);
        }
    }

    //Moving a castling rook
    constexpr BoardStatus RookMove_Left() const {
        if (WhiteMove) {
            return BoardStatus(!WhiteMove, false, false, WCastleR, BCastleL, BCastleR);
        }
        else {
            return BoardStatus(!WhiteMove, false, WCastleL, WCastleR, false, BCastleR);
        }
    }

    constexpr BoardStatus RookMove_Right() const {
        if (WhiteMove) {
            return BoardStatus(!WhiteMove, false, WCastleL, false, BCastleL, BCastleR);
        }
        else {
            return BoardStatus(!WhiteMove, false, WCastleL, WCastleR, BCastleL, false);
        }
    }


    constexpr BoardStatus SilentMove() const {
        return BoardStatus(!WhiteMove, false, WCastleL, WCastleR, BCastleL, BCastleR);
    }

    static constexpr BoardStatus Default() {
        //White, no EP, not in check, all castling rights
        return BoardStatus(true, false, true, true, true, true);
    }




};

std::ostream& operator<<(std::ostream& os, const BoardStatus& dt);
std::ostream& operator<<(std::ostream& os, const BoardStatus& dt)
{
    if (dt.WhiteMove) os << 'w';
    else os << "b";

    if (dt.HasEPPawn) os << " ep:1";
    else os << " ep:0";

    if (!dt.WCastleL && !dt.WCastleR && !dt.BCastleL && !dt.BCastleR)
    {
        os << " castle:0";
    }
    else {
        os << " castle:";
        if (dt.WCastleL) os << "Q";
        if (dt.WCastleR) os << "K";
        if (dt.BCastleL) os << "q";
        if (dt.BCastleR) os << "k";
    }
    return os;
}

enum class FenField {
    white,
    hasEP,
    WCastleL,
    WCastleR,
    BCastleL,
    BCastleR
};



static std::string _map(uint64_t val1, uint64_t val2, uint64_t val3)
{
    static std::string str(64 * 3 + 3 * 8, 'o'); //2 boards with space between

    for (uint64_t i = 0, c = 0; i < 64; i++)
    {
        uint64_t bitmask = (1ull << 63) >> i;

        if ((bitmask & val1) != 0) str[c] = 'X';
        else str[c] = '.';

        if ((bitmask & val2) != 0) str[c + 9] = 'X';
        else str[c + 9] = '.';

        if ((bitmask & val3) != 0) str[c + 18] = 'X';
        else str[c + 18] = '.';

        c++;

        if ((i + 1) % 8 == 0) {
            str[c] = ' ';
            str[c + 9] = ' ';
            str[c + 18] = '\n';
            c += 19;
        }
    }
    return str;
}

static std::string _map(uint64_t val1, uint64_t val2)
{
    static std::string str(64 * 2 + 2 * 8, 'o'); //2 boards with space between

    for (uint64_t i = 0, c = 0; i < 64; i++)
    {
        uint64_t bitmask = (1ull << 63) >> i;

        if ((bitmask & val1) != 0) str[c] = 'X';
        else str[c] = '.';

        if ((bitmask & val2) != 0) str[c + 9] = 'X';
        else str[c + 9] = '.';

        c++;


        if ((i + 1) % 8 == 0) {
            str[c] = ' ';
            str[c + 9] = '\n';
            c += 10;
        }
    }
    return str;
}

static std::string _map(uint64_t value)
{
    static std::string str(64 + 8, 'o');
    for (uint64_t i = 0, c = 0; i < 64; i++)
    {
        uint64_t bitmask = (1ull << 63) >> i;

        if ((bitmask & value) != 0) str[c++] = 'X';
        else str[c++] = '.';

        if ((i + 1) % 8 == 0) str[c++] = '\n';
    }
    return str;
}

struct FEN {
    static constexpr uint64_t FenEnpassant(std::string_view FEN) {
        uint64_t i = 0;

        while (FEN[i++] != ' ')
        {

        }
        char wb = FEN[i++];
        i++;

        //Castling
        while (FEN[i++] != ' ')
        {

        }

        //En Passant
        char EorMinus = FEN[i++];
        if (EorMinus != '-') {
            if (wb == 'w') {
                //Todo where to store Enpassant
                int EPpos = 32 + ('h' - EorMinus);
                return 1ull << EPpos;
            }
            if (wb == 'b') {
                //Todo where to store Enpassant
                int EPpos = 24 + ('h' - EorMinus);
                return 1ull << EPpos;
            }
        }
        return 0;
    }


    template<FenField field>
    static constexpr bool FenInfo(std::string_view FEN) {
        uint64_t i = 0;
        char c{};

        while ((c = FEN[i++]) != ' ')
        {

        }
        char wb = FEN[i++];

        //White
        if constexpr (field == FenField::white) {
            if (wb == 'w') return true;
            else return false;
        }
        i++;

        //Castling
        while ((c = FEN[i++]) != ' ')
        {
            if constexpr (field == FenField::WCastleR) {
                if (c == 'K') return true;
            }
            if constexpr (field == FenField::WCastleL) {
                if (c == 'Q') return true;
            }
            if constexpr (field == FenField::BCastleR) {
                if (c == 'k') return true;
            }
            if constexpr (field == FenField::BCastleL) {
                if (c == 'q') return true;
            }
        }
        if constexpr (field == FenField::WCastleR || field == FenField::WCastleL || field == FenField::BCastleR || field == FenField::BCastleL)
        {
            return false;
        }

        //En Passant
        char EorMinus = FEN[i++];
        if (EorMinus != '-') {
            if (wb == 'w') {
                //Todo where to store Enpassant
                //int EPpos = 32 + ('h' - EorMinus);
                if constexpr (field == FenField::hasEP) return true;
            }
            if (wb == 'b') {
                //Todo where to store Enpassant
                //int EPpos = 24 + ('h' - EorMinus);
                if constexpr (field == FenField::hasEP) return true;
            }
        }
        if constexpr (field == FenField::hasEP) return false;
    }

    /// Transform FEN character 'n' or 'Q' into bitmap where the bits correspond to the field
    static constexpr uint64_t FenToBmp(std::string_view FEN, char p)
    {
        uint64_t i = 0;
        char c{};
        int Field = 63;

        uint64_t result = 0;
        while ((c = FEN[i++]) != ' ')
        {
            uint64_t P = 1ull << Field;
            switch (c) {
            case '/': Field += 1; break;
            case '1': break;
            case '2': Field -= 1; break;
            case '3': Field -= 2; break;
            case '4': Field -= 3; break;
            case '5': Field -= 4; break;
            case '6': Field -= 5; break;
            case '7': Field -= 6; break;
            case '8': Field -= 7; break;
            default:
                if (c == p) result |= P; //constexpr parsing happens here
            }
            Field--;
        }
        return result;
    }
};

enum class BoardPiece {
    Pawn, Knight, Bishop, Rook, Queen, King
};


struct Board {
    const map BPawn;
    const map BKnight;
    const map BBishop;
    const map BRook;
    const map BQueen;
    const map BKing;

    const map WPawn;
    const map WKnight;
    const map WBishop;
    const map WRook;
    const map WQueen;
    const map WKing;

    const map Black;
    const map White;
    const map Occ;

    constexpr Board(
        map bp, map bn, map bb, map br, map bq, map bk,
        map wp, map wn, map wb, map wr, map wq, map wk) :
        BPawn(bp), BKnight(bn), BBishop(bb), BRook(br), BQueen(bq), BKing(bk),
        WPawn(wp), WKnight(wn), WBishop(wb), WRook(wr), WQueen(wq), WKing(wk),
        Black(bp | bn | bb | br | bq | bk),
        White(wp | wn | wb | wr | wq | wk),
        Occ(Black | White)
    {

    }

    constexpr Board(std::string_view FEN) : 
        Board(FEN::FenToBmp(FEN, 'p'), FEN::FenToBmp(FEN, 'n'), FEN::FenToBmp(FEN, 'b'), FEN::FenToBmp(FEN, 'r'), FEN::FenToBmp(FEN, 'q'), FEN::FenToBmp(FEN, 'k'),
            FEN::FenToBmp(FEN, 'P'), FEN::FenToBmp(FEN, 'N'), FEN::FenToBmp(FEN, 'B'), FEN::FenToBmp(FEN, 'R'), FEN::FenToBmp(FEN, 'Q'), FEN::FenToBmp(FEN, 'K'))
    {
        
    }

    template<BoardPiece piece, bool IsWhite>
    _Compiletime Board MovePromote(const Board& existing, uint64_t from, uint64_t to)
    {
        const uint64_t rem = ~to;
        const map bp = existing.BPawn;
        const map bn = existing.BKnight;
        const map bb = existing.BBishop;
        const map br = existing.BRook;
        const map bq = existing.BQueen;
        const map bk = existing.BKing;

        const map wp = existing.WPawn;
        const map wn = existing.WKnight;
        const map wb = existing.WBishop;
        const map wr = existing.WRook;
        const map wq = existing.WQueen;
        const map wk = existing.WKing;

        if constexpr (IsWhite) {
            if constexpr (BoardPiece::Queen == piece)  return Board(bp & rem, bn & rem, bb & rem, br & rem, bq & rem, bk, wp ^ from, wn, wb, wr, wq ^ to, wk);
            if constexpr (BoardPiece::Rook == piece)   return Board(bp & rem, bn & rem, bb & rem, br & rem, bq & rem, bk, wp ^ from, wn, wb, wr ^ to, wq, wk);
            if constexpr (BoardPiece::Bishop == piece) return Board(bp & rem, bn & rem, bb & rem, br & rem, bq & rem, bk, wp ^ from, wn, wb ^ to, wr, wq, wk);
            if constexpr (BoardPiece::Knight == piece) return Board(bp & rem, bn & rem, bb & rem, br & rem, bq & rem, bk, wp ^ from, wn ^ to, wb, wr, wq, wk);
        }
        else {
            if constexpr (BoardPiece::Queen == piece)  return Board(bp ^ from, bn, bb, br, bq ^ to, bk, wp & rem, wn & rem, wb & rem, wr & rem, wq & rem, wk);
            if constexpr (BoardPiece::Rook == piece)   return Board(bp ^ from, bn, bb, br ^ to, bq, bk, wp & rem, wn & rem, wb & rem, wr & rem, wq & rem, wk);
            if constexpr (BoardPiece::Bishop == piece) return Board(bp ^ from, bn, bb ^ to, br, bq, bk, wp & rem, wn & rem, wb & rem, wr & rem, wq & rem, wk);
            if constexpr (BoardPiece::Knight == piece) return Board(bp ^ from, bn ^ to, bb, br, bq, bk, wp & rem, wn & rem, wb & rem, wr & rem, wq & rem, wk);
        }
    }
    


    //Todo: elegant not code duplication for Castling
    template<bool IsWhite>
    _Compiletime Board MoveCastle(const Board& existing, uint64_t kingswitch, uint64_t rookswitch)
    {
        const map bp = existing.BPawn;
        const map bn = existing.BKnight;
        const map bb = existing.BBishop;
        const map br = existing.BRook;
        const map bq = existing.BQueen;
        const map bk = existing.BKing;

        const map wp = existing.WPawn;
        const map wn = existing.WKnight;
        const map wb = existing.WBishop;
        const map wr = existing.WRook;
        const map wq = existing.WQueen;
        const map wk = existing.WKing;

        if constexpr (IsWhite) {
            return Board(bp, bn, bb, br, bq, bk, wp, wn, wb, wr ^ rookswitch, wq, wk ^ kingswitch);
        }
        else {
            return Board(bp, bn, bb, br ^ rookswitch, bq, bk ^ kingswitch, wp, wn, wb, wr, wq, wk);
        }
    }

    //Todo: elegant not code duplication for EP taking. Where to and rem are different squares
    template<bool IsWhite>
    _Compiletime Board MoveEP(const Board& existing, uint64_t from, uint64_t enemy, uint64_t to)
    {
        const uint64_t rem = ~enemy;
        const map bp = existing.BPawn;
        const map bn = existing.BKnight;
        const map bb = existing.BBishop;
        const map br = existing.BRook;
        const map bq = existing.BQueen;
        const map bk = existing.BKing;

        const map wp = existing.WPawn;
        const map wn = existing.WKnight;
        const map wb = existing.WBishop;
        const map wr = existing.WRook;
        const map wq = existing.WQueen;
        const map wk = existing.WKing;
        const map mov = from | to;


        if constexpr (IsWhite) {
            return Board(bp & rem, bn & rem, bb & rem, br & rem, bq & rem, bk, wp ^ mov, wn, wb, wr, wq, wk);
        }
        else {
            return Board(bp ^ mov, bn, bb, br, bq, bk, wp & rem, wn & rem, wb & rem, wr & rem, wq & rem, wk);
        }
    }

#ifdef _DEBUG
    _Inline uint64_t BlackPieceCount() const {
        return Bitcount(Black);
    }
    _Inline uint64_t WhitePiecesCount() const {
        return Bitcount(White);
    }
    _Inline uint64_t AllPiecesCount() const {
        return Bitcount(Occ);
    }

    template<bool IsWhite>
    _Inline static void AssertBoardMove(const Board& before, const Board& after, bool IsTaking)
    {
        m_assert(before.BlackPieceCount() + before.WhitePiecesCount() == before.AllPiecesCount(), "Some squares are taken twice!");
        m_assert(after.BlackPieceCount() + after.WhitePiecesCount() == after.AllPiecesCount(), "Some squares are taken twice after a move!");
        if (IsTaking) {
            m_assert(before.AllPiecesCount() == after.AllPiecesCount() + 1, "Piece count did not decrease by one after taking!");
        }
        else {
            m_assert(before.AllPiecesCount() == after.AllPiecesCount(), "Piece count did not stay the same after a move!");
        }
        m_assert(after.WKing != 0, "White King is missing!");
        m_assert(after.BKing != 0, "Black King is missing!");

        int kingpos = SquareOf(King<IsWhite>(after));

        //In chess you cannot be in check after your own move
        map InvalidCheck1 = Lookup::Bishop(kingpos, after.Occ)& EnemyBishopQueen<IsWhite>(after);
        map InvalidCheck2 = Lookup::Rook(kingpos, after.Occ)& EnemyRookQueen<IsWhite>(after);
        map InvalidCheck3 = Lookup::Knight(kingpos) & Knights<!IsWhite>(after);
        m_assert(InvalidCheck1 == 0, "Still in Check by a bishop/queen after the move");
        m_assert(InvalidCheck2 == 0, "Still in Check by a rook/queen after the move");
        m_assert(InvalidCheck3 == 0, "Still in Check by a knight after the move");
    }
#endif 


    template<BoardPiece piece, bool IsWhite>
    _Compiletime Board Move(const Board& existing, uint64_t from, uint64_t to, bool IsTaking)
    {
        if (IsTaking) return Move<piece, IsWhite, true>(existing, from, to);
        else return Move<piece, IsWhite, false>(existing, from, to);
    }

    template<BoardPiece piece, bool IsWhite, bool IsTaking>
    _Compiletime Board Move(const Board& existing, uint64_t from, uint64_t to)
    {
        const map bp = existing.BPawn; const map bn = existing.BKnight; const map bb = existing.BBishop; const map br = existing.BRook; const map bq = existing.BQueen; const map bk = existing.BKing;
        const map wp = existing.WPawn; const map wn = existing.WKnight; const map wb = existing.WBishop; const map wr = existing.WRook; const map wq = existing.WQueen; const map wk = existing.WKing;
        
        const map mov = from | to;
        
        if constexpr (IsTaking)
        {
            const uint64_t rem = ~to;
            if constexpr (IsWhite) {
                m_assert((bk & mov) == 0, "Taking Black King is not legal!");
                m_assert((to & existing.White) == 0, "Cannot move to square of same white color!");
                if constexpr (BoardPiece::Pawn == piece)    return Board(bp & rem, bn & rem, bb & rem, br & rem, bq & rem, bk, wp ^ mov, wn, wb, wr, wq, wk);
                if constexpr (BoardPiece::Knight == piece)  return Board(bp & rem, bn & rem, bb & rem, br & rem, bq & rem, bk, wp, wn ^ mov, wb, wr, wq, wk);
                if constexpr (BoardPiece::Bishop == piece)  return Board(bp & rem, bn & rem, bb & rem, br & rem, bq & rem, bk, wp, wn, wb ^ mov, wr, wq, wk);
                if constexpr (BoardPiece::Rook == piece)    return Board(bp & rem, bn & rem, bb & rem, br & rem, bq & rem, bk, wp, wn, wb, wr ^ mov, wq, wk);
                if constexpr (BoardPiece::Queen == piece)   return Board(bp & rem, bn & rem, bb & rem, br & rem, bq & rem, bk, wp, wn, wb, wr, wq ^ mov, wk);
                if constexpr (BoardPiece::King == piece)    return Board(bp & rem, bn & rem, bb & rem, br & rem, bq & rem, bk, wp, wn, wb, wr, wq, wk ^ mov);
            }
            else {
                m_assert((wk & mov) == 0, "Taking White King is not legal!");
                m_assert((to & existing.Black) == 0, "Cannot move to square of same black color!");
                if constexpr (BoardPiece::Pawn == piece)    return Board(bp ^ mov, bn, bb, br, bq, bk, wp & rem, wn & rem, wb & rem, wr & rem, wq & rem, wk);
                if constexpr (BoardPiece::Knight == piece)  return Board(bp, bn ^ mov, bb, br, bq, bk, wp & rem, wn & rem, wb & rem, wr & rem, wq & rem, wk);
                if constexpr (BoardPiece::Bishop == piece)  return Board(bp, bn, bb ^ mov, br, bq, bk, wp & rem, wn & rem, wb & rem, wr & rem, wq & rem, wk);
                if constexpr (BoardPiece::Rook == piece)    return Board(bp, bn, bb, br ^ mov, bq, bk, wp & rem, wn & rem, wb & rem, wr & rem, wq & rem, wk);
                if constexpr (BoardPiece::Queen == piece)   return Board(bp, bn, bb, br, bq ^ mov, bk, wp & rem, wn & rem, wb & rem, wr & rem, wq & rem, wk);
                if constexpr (BoardPiece::King == piece)    return Board(bp, bn, bb, br, bq, bk ^ mov, wp & rem, wn & rem, wb & rem, wr & rem, wq & rem, wk);
            }
        }
        else {
            if constexpr (IsWhite) {
                m_assert((bk & mov) == 0, "Taking Black King is not legal!");
                m_assert((to & existing.White) == 0, "Cannot move to square of same white color!");
                if constexpr (BoardPiece::Pawn == piece)    return Board(bp, bn, bb, br, bq, bk, wp ^ mov, wn, wb, wr, wq, wk);
                if constexpr (BoardPiece::Knight == piece)  return Board(bp, bn, bb, br, bq, bk, wp, wn ^ mov, wb, wr, wq, wk);
                if constexpr (BoardPiece::Bishop == piece)  return Board(bp, bn, bb, br, bq, bk, wp, wn, wb ^ mov, wr, wq, wk);
                if constexpr (BoardPiece::Rook == piece)    return Board(bp, bn, bb, br, bq, bk, wp, wn, wb, wr ^ mov, wq, wk);
                if constexpr (BoardPiece::Queen == piece)   return Board(bp, bn, bb, br, bq, bk, wp, wn, wb, wr, wq ^ mov, wk);
                if constexpr (BoardPiece::King == piece)    return Board(bp, bn, bb, br, bq, bk, wp, wn, wb, wr, wq, wk ^ mov);
            }
            else {
                m_assert((wk & mov) == 0, "Taking White King is not legal!");
                m_assert((to & existing.Black) == 0, "Cannot move to square of same black color!");
                if constexpr (BoardPiece::Pawn == piece)    return Board(bp ^ mov, bn, bb, br, bq, bk, wp, wn, wb, wr, wq, wk);
                if constexpr (BoardPiece::Knight == piece)  return Board(bp, bn ^ mov, bb, br, bq, bk, wp, wn, wb, wr, wq, wk);
                if constexpr (BoardPiece::Bishop == piece)  return Board(bp, bn, bb ^ mov, br, bq, bk, wp, wn, wb, wr, wq, wk);
                if constexpr (BoardPiece::Rook == piece)    return Board(bp, bn, bb, br ^ mov, bq, bk, wp, wn, wb, wr, wq, wk);
                if constexpr (BoardPiece::Queen == piece)   return Board(bp, bn, bb, br, bq ^ mov, bk, wp, wn, wb, wr, wq, wk);
                if constexpr (BoardPiece::King == piece)    return Board(bp, bn, bb, br, bq, bk ^ mov, wp, wn, wb, wr, wq, wk);
            }
        }
        
    }


    static constexpr Board Default() {
        return Board("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    }
};

static char MapBitToChar(uint64_t bit, const Board& brd)
{
    if (bit & brd.BPawn)    return 'p';
    if (bit & brd.BKnight)  return 'n';
    if (bit & brd.BBishop)  return 'b';
    if (bit & brd.BRook)    return 'r';
    if (bit & brd.BQueen)   return 'q';
    if (bit & brd.BKing)    return 'k';
    if (bit & brd.WPawn)    return 'P';
    if (bit & brd.WKnight)  return 'N';
    if (bit & brd.WBishop)  return 'B';
    if (bit & brd.WRook)    return 'R';
    if (bit & brd.WQueen)   return 'Q';
    if (bit & brd.WKing)    return 'K';
    return '.';
}

static std::string _map(uint64_t val1, uint64_t val2, const Board& val3, const Board& val4)
{
    static std::string str(64 * 4 + 4 * 8, 'o'); //2 boards with space between

    for (uint64_t i = 0, c = 0; i < 64; i++)
    {
        uint64_t bitmask = (1ull << 63) >> i;

        if ((bitmask & val1) != 0) str[c] = 'X';
        else str[c] = '.';

        if ((bitmask & val2) != 0) str[c + 9] = 'X';
        else str[c + 9] = '.';

        str[c + 18] = MapBitToChar(bitmask, val3);
        str[c + 27] = MapBitToChar(bitmask, val4);

        c++;

        if ((i + 1) % 8 == 0) {
            str[c] = ' ';
            str[c + 9] = ' ';
            str[c + 18] = ' ';
            str[c + 27] = '\n';
            c += 28;
        }
    }
    return str;
}

/// <summary>
/// Call this via _func(brd)
/// </summary>
#define PositionToTemplate(func) \
static inline void _##func(std::string_view pos, int depth) { \
const bool WH = FEN::FenInfo<FenField::white>(pos);\
const bool EP = FEN::FenInfo<FenField::hasEP>(pos);\
const bool BL = FEN::FenInfo<FenField::BCastleL>(pos);\
const bool BR = FEN::FenInfo<FenField::BCastleR>(pos);\
const bool WL = FEN::FenInfo<FenField::WCastleL>(pos);\
const bool WR = FEN::FenInfo<FenField::WCastleR>(pos);\
Board brd(pos);\
if ( WH &&  EP &&  WL &&  WR &&  BL &&  BR)       return func<BoardStatus(0b111111)>(pos, brd, depth); \
if ( WH &&  EP &&  WL &&  WR &&  BL && !BR)       return func<BoardStatus(0b111110)>(pos, brd, depth); \
if ( WH &&  EP &&  WL &&  WR && !BL &&  BR)       return func<BoardStatus(0b111101)>(pos, brd, depth); \
if ( WH &&  EP &&  WL &&  WR && !BL && !BR)       return func<BoardStatus(0b111100)>(pos, brd, depth); \
if ( WH &&  EP &&  WL && !WR &&  BL &&  BR)       return func<BoardStatus(0b111011)>(pos, brd, depth); \
if ( WH &&  EP &&  WL && !WR &&  BL && !BR)       return func<BoardStatus(0b111010)>(pos, brd, depth); \
if ( WH &&  EP &&  WL && !WR && !BL &&  BR)       return func<BoardStatus(0b111001)>(pos, brd, depth); \
if ( WH &&  EP &&  WL && !WR && !BL && !BR)       return func<BoardStatus(0b111000)>(pos, brd, depth); \
if ( WH &&  EP && !WL &&  WR &&  BL &&  BR)       return func<BoardStatus(0b110111)>(pos, brd, depth); \
if ( WH &&  EP && !WL &&  WR &&  BL && !BR)       return func<BoardStatus(0b110110)>(pos, brd, depth); \
if ( WH &&  EP && !WL &&  WR && !BL &&  BR)       return func<BoardStatus(0b110101)>(pos, brd, depth); \
if ( WH &&  EP && !WL &&  WR && !BL && !BR)       return func<BoardStatus(0b110100)>(pos, brd, depth); \
if ( WH &&  EP && !WL && !WR &&  BL &&  BR)       return func<BoardStatus(0b110011)>(pos, brd, depth); \
if ( WH &&  EP && !WL && !WR &&  BL && !BR)       return func<BoardStatus(0b110010)>(pos, brd, depth); \
if ( WH &&  EP && !WL && !WR && !BL &&  BR)       return func<BoardStatus(0b110001)>(pos, brd, depth); \
if ( WH &&  EP && !WL && !WR && !BL && !BR)       return func<BoardStatus(0b110000)>(pos, brd, depth); \
if ( WH && !EP &&  WL &&  WR &&  BL &&  BR)       return func<BoardStatus(0b101111)>(pos, brd, depth); \
if ( WH && !EP &&  WL &&  WR &&  BL && !BR)       return func<BoardStatus(0b101110)>(pos, brd, depth); \
if ( WH && !EP &&  WL &&  WR && !BL &&  BR)       return func<BoardStatus(0b101101)>(pos, brd, depth); \
if ( WH && !EP &&  WL &&  WR && !BL && !BR)       return func<BoardStatus(0b101100)>(pos, brd, depth); \
if ( WH && !EP &&  WL && !WR &&  BL &&  BR)       return func<BoardStatus(0b101011)>(pos, brd, depth); \
if ( WH && !EP &&  WL && !WR &&  BL && !BR)       return func<BoardStatus(0b101010)>(pos, brd, depth); \
if ( WH && !EP &&  WL && !WR && !BL &&  BR)       return func<BoardStatus(0b101001)>(pos, brd, depth); \
if ( WH && !EP &&  WL && !WR && !BL && !BR)       return func<BoardStatus(0b101000)>(pos, brd, depth); \
if ( WH && !EP && !WL &&  WR &&  BL &&  BR)       return func<BoardStatus(0b100111)>(pos, brd, depth); \
if ( WH && !EP && !WL &&  WR &&  BL && !BR)       return func<BoardStatus(0b100110)>(pos, brd, depth); \
if ( WH && !EP && !WL &&  WR && !BL &&  BR)       return func<BoardStatus(0b100101)>(pos, brd, depth); \
if ( WH && !EP && !WL &&  WR && !BL && !BR)       return func<BoardStatus(0b100100)>(pos, brd, depth); \
if ( WH && !EP && !WL && !WR &&  BL &&  BR)       return func<BoardStatus(0b100011)>(pos, brd, depth); \
if ( WH && !EP && !WL && !WR &&  BL && !BR)       return func<BoardStatus(0b100010)>(pos, brd, depth); \
if ( WH && !EP && !WL && !WR && !BL &&  BR)       return func<BoardStatus(0b100001)>(pos, brd, depth); \
if ( WH && !EP && !WL && !WR && !BL && !BR)       return func<BoardStatus(0b100000)>(pos, brd, depth); \
if (!WH &&  EP &&  WL &&  WR &&  BL &&  BR)       return func<BoardStatus(0b011111)>(pos, brd, depth); \
if (!WH &&  EP &&  WL &&  WR &&  BL && !BR)       return func<BoardStatus(0b011110)>(pos, brd, depth); \
if (!WH &&  EP &&  WL &&  WR && !BL &&  BR)       return func<BoardStatus(0b011101)>(pos, brd, depth); \
if (!WH &&  EP &&  WL &&  WR && !BL && !BR)       return func<BoardStatus(0b011100)>(pos, brd, depth); \
if (!WH &&  EP &&  WL && !WR &&  BL &&  BR)       return func<BoardStatus(0b011011)>(pos, brd, depth); \
if (!WH &&  EP &&  WL && !WR &&  BL && !BR)       return func<BoardStatus(0b011010)>(pos, brd, depth); \
if (!WH &&  EP &&  WL && !WR && !BL &&  BR)       return func<BoardStatus(0b011001)>(pos, brd, depth); \
if (!WH &&  EP &&  WL && !WR && !BL && !BR)       return func<BoardStatus(0b011000)>(pos, brd, depth); \
if (!WH &&  EP && !WL &&  WR &&  BL &&  BR)       return func<BoardStatus(0b010111)>(pos, brd, depth); \
if (!WH &&  EP && !WL &&  WR &&  BL && !BR)       return func<BoardStatus(0b010110)>(pos, brd, depth); \
if (!WH &&  EP && !WL &&  WR && !BL &&  BR)       return func<BoardStatus(0b010101)>(pos, brd, depth); \
if (!WH &&  EP && !WL &&  WR && !BL && !BR)       return func<BoardStatus(0b010100)>(pos, brd, depth); \
if (!WH &&  EP && !WL && !WR &&  BL &&  BR)       return func<BoardStatus(0b010011)>(pos, brd, depth); \
if (!WH &&  EP && !WL && !WR &&  BL && !BR)       return func<BoardStatus(0b010010)>(pos, brd, depth); \
if (!WH &&  EP && !WL && !WR && !BL &&  BR)       return func<BoardStatus(0b010001)>(pos, brd, depth); \
if (!WH &&  EP && !WL && !WR && !BL && !BR)       return func<BoardStatus(0b010000)>(pos, brd, depth); \
if (!WH && !EP &&  WL &&  WR &&  BL &&  BR)       return func<BoardStatus(0b001111)>(pos, brd, depth); \
if (!WH && !EP &&  WL &&  WR &&  BL && !BR)       return func<BoardStatus(0b001110)>(pos, brd, depth); \
if (!WH && !EP &&  WL &&  WR && !BL &&  BR)       return func<BoardStatus(0b001101)>(pos, brd, depth); \
if (!WH && !EP &&  WL &&  WR && !BL && !BR)       return func<BoardStatus(0b001100)>(pos, brd, depth); \
if (!WH && !EP &&  WL && !WR &&  BL &&  BR)       return func<BoardStatus(0b001011)>(pos, brd, depth); \
if (!WH && !EP &&  WL && !WR &&  BL && !BR)       return func<BoardStatus(0b001010)>(pos, brd, depth); \
if (!WH && !EP &&  WL && !WR && !BL &&  BR)       return func<BoardStatus(0b001001)>(pos, brd, depth); \
if (!WH && !EP &&  WL && !WR && !BL && !BR)       return func<BoardStatus(0b001000)>(pos, brd, depth); \
if (!WH && !EP && !WL &&  WR &&  BL &&  BR)       return func<BoardStatus(0b000111)>(pos, brd, depth); \
if (!WH && !EP && !WL &&  WR &&  BL && !BR)       return func<BoardStatus(0b000110)>(pos, brd, depth); \
if (!WH && !EP && !WL &&  WR && !BL &&  BR)       return func<BoardStatus(0b000101)>(pos, brd, depth); \
if (!WH && !EP && !WL &&  WR && !BL && !BR)       return func<BoardStatus(0b000100)>(pos, brd, depth); \
if (!WH && !EP && !WL && !WR &&  BL &&  BR)       return func<BoardStatus(0b000011)>(pos, brd, depth); \
if (!WH && !EP && !WL && !WR &&  BL && !BR)       return func<BoardStatus(0b000010)>(pos, brd, depth); \
if (!WH && !EP && !WL && !WR && !BL &&  BR)       return func<BoardStatus(0b000001)>(pos, brd, depth); \
if (!WH && !EP && !WL && !WR && !BL && !BR)       return func<BoardStatus(0b000000)>(pos, brd, depth); \
return func<BoardStatus::Default()>(pos, brd, depth);}


