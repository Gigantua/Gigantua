#pragma once
#include "Chess_Base.hpp"
#include "Movegen.hpp"



namespace Movestack 
{
    //Can be removed - incremental bitboard to save some slider lookups is more expensive then lookup itself. So this release does not have a changemap
    static inline Square Atk_King[32];  //Current moves for current King
    static inline Square Atk_EKing[32]; //Current enemy king attacked squares

    static inline map Check_Status[32];   //When a pawn or a knight does check we can assume at least one check. And only one (initially) since a pawn or knight cannot do discovery
}


namespace Movelist {
    //move = atkmap + enemyorempty + checkmask + pins 
    map EnPassantTarget = { }; //Where the current EP Target is. Only valid if the movestatus contains EP flag. 

    //These fields change during enumeration - so we have to copy them to a local variable!
    map RookPin = { }; //Pins that run in rank or file direction - important because a queen can see two pins at once: https://lichess.org/editor?fen=3r4%2F8%2F8%2F3P4%2F3K1Q1r%2F8%2F8%2F8+w+-+-+0+1
    map BishopPin = { }; //Pins that run in diagonal direction


    template<class BoardStatus status, int depth>
    _ForceInline void InitStack(Board& brd) {
        constexpr bool white = status.WhiteMove;
        constexpr bool enemy = !status.WhiteMove;
        //Movestack::Kingpos[depth] = SquareOf(King<status.WhiteMove>(brd));
        Movestack::Atk_King[depth] =  Lookup::King(SquareOf(King<white>(brd)));
        Movestack::Atk_EKing[depth] = Lookup::King(SquareOf(King<enemy>(brd)));

        //Calculate Check from enemy pawns
        {
            const map pl = Pawn_AttackLeft<enemy>(Pawns<enemy>(brd) & Pawns_NotLeft());
            const map pr = Pawn_AttackRight<enemy>(Pawns<enemy>(brd) & Pawns_NotRight());

            if (pl & King<white>(brd)) Movestack::Check_Status[depth] = Pawn_AttackRight<white>(King<white>(brd));
            else if (pr & King<white>(brd)) Movestack::Check_Status[depth] = Pawn_AttackLeft<white>(King<white>(brd));
            else Movestack::Check_Status[depth] = 0xffffffffffffffffull;
        }

        //Calculate Check from enemy knights
        {
            map knightcheck = Lookup::Knight(SquareOf(King<white>(brd))) & Knights<enemy>(brd);
            if (knightcheck) Movestack::Check_Status[depth] = knightcheck;
        }

        //We are assuming only normal positions to arise. So no checks by two knights/pawns at once. There HAS to be a slider involved in doublecheck. No two knights or pawn/knight can check at once.

    }


    /// <summary>
    /// Sets up the list for a completely new enumeration
    /// </summary>
    _ForceInline void Init(map EPInit) {
        Movelist::EnPassantTarget = EPInit; //EPSuare is not a member of the template
    }


    //PinHVD1D2 |= Path from Enemy to excluding King + enemy. Has Seemap from king as input
    //Must have enemy slider AND own piece or - VERY special: can clear enemy enpassant pawn
    template<class BoardStatus status>
    _ForceInline void RegisterPinD12(Square king, Square enemy, Board& brd, map& EPTarget)
    {
        const map Pinmask = Chess_Lookup::PinBetween[king * 64 + enemy];

        //Deep possible chess problem:
        //Enemy Enpassant Pawn gives check - Could be taken by EP Pawn - But is pinned and therefore not a valid EPTarget
        // https://lichess.org/editor?fen=6q1%2F8%2F8%2F3pP3%2F8%2F1K6%2F8%2F8+w+-+-+0+1
        //Can delete EP Status
        if constexpr (status.HasEPPawn) {
            if (Pinmask & EPTarget) EPTarget = 0;
        }

        if (Pinmask & OwnColor<status.WhiteMove>(brd)) {
            BishopPin |= Pinmask;
        }
    }

    //PinHVD1D2 |= Path from Enemy to excluding King + enemy. Has Seemap from king as input
    template<class BoardStatus status>
    _ForceInline void RegisterPinHV(Square king, Square enemy, Board& brd)
    {
        const map Pinmask = Chess_Lookup::PinBetween[king * 64 + enemy];

        if (Pinmask & OwnColor<status.WhiteMove>(brd)) {
            RookPin |= Pinmask;
        }
    }


    template<bool IsWhite>
    _Compiletime map EPRank()
    {
        if constexpr (IsWhite) return 0xFFull << 32;
        else return 0xFFull << 24;
    }

    template<class BoardStatus status>
    _ForceInline void RegisterPinEP(Square kingsquare, Bit king, map enemyRQ, Board& brd)
    {
        constexpr bool white = status.WhiteMove;
        const map pawns = Pawns<status.WhiteMove>(brd);
        //Special Horizontal1 https://lichess.org/editor?fen=8%2F8%2F8%2F1K1pP1q1%2F8%2F8%2F8%2F8+w+-+-+0+1
        //Special Horizontal2 https://lichess.org/editor?fen=8%2F8%2F8%2F1K1pP1q1%2F8%2F8%2F8%2F8+w+-+-+0+1

        //King is on EP rank and enemy HV walker is on same rank

        //Remove enemy EP and own EP Candidate from OCC and check if Horizontal path to enemy Slider is open
        //Quick check: We have king - Enemy Slider - Own Pawn - and enemy EP on the same rank!
        if ((EPRank<white>() & king) && (EPRank<white>() & enemyRQ) && (EPRank<white>() & pawns))
        {
            Bit EPLpawn = pawns & ((EnPassantTarget & Pawns_NotRight()) >> 1); //Pawn that can EPTake to the left
            Bit EPRpawn = pawns & ((EnPassantTarget & Pawns_NotLeft()) << 1);  //Pawn that can EPTake to the right

            if (EPLpawn) {
                map AfterEPocc = brd.Occ & ~(EnPassantTarget | EPLpawn);
                if ((Lookup::Rook(kingsquare, AfterEPocc) & EPRank<white>()) & enemyRQ) EnPassantTarget = 0;
            }
            if (EPRpawn) {
                map AfterEPocc = brd.Occ & ~(EnPassantTarget | EPRpawn);
                if ((Lookup::Rook(kingsquare, AfterEPocc) & EPRank<white>()) & enemyRQ) EnPassantTarget = 0;
            }
        }
    }

    //Checkmask |= Path from Enemy to including King + square behind king + enemy //Todo: make this unconditional again!
    _ForceInline void CheckBySlider(Square king, Square enemy, map& Kingban, map& Checkmask) {
        if (Checkmask == 0xffffffffffffffffull)
        {
            Checkmask = Chess_Lookup::PinBetween[king * 64 + enemy]; //Checks are only stopped between king and enemy including taking the enemy
        }
        else Checkmask = 0;
        Kingban |= Chess_Lookup::CheckBetween[king * 64 + enemy]; //King cannot go to square opposite to slider
    }


    template<class BoardStatus status, int depth>
    _ForceInline map Refresh(Board& brd, map& kingban, map& checkmask)
    {
        constexpr bool white = status.WhiteMove;
        constexpr bool enemy = !status.WhiteMove;
        const Bit king = King<white>(brd);
        const map kingsq = SquareOf(king);

        
        //Pinned pieces + checks by sliders
        {
            RookPin = 0; BishopPin = 0;
            
            if (Chess_Lookup::RookMask[kingsq] & EnemyRookQueen<white>(brd))
            {
                map atkHV = Lookup::Rook(kingsq, brd.Occ) & EnemyRookQueen<white>(brd);
                Bitloop(atkHV) {
                    Square sq = SquareOf(atkHV);
                    CheckBySlider(kingsq, sq, kingban, checkmask);
                }

                map pinnersHV = Lookup::Rook_Xray(kingsq, brd.Occ) & EnemyRookQueen<white>(brd);
                Bitloop(pinnersHV)
                {
                    RegisterPinHV<status>(kingsq, SquareOf(pinnersHV), brd);
                }
            }
            if (Chess_Lookup::BishopMask[kingsq] & EnemyBishopQueen<white>(brd)) {
                map atkD12 = Lookup::Bishop(kingsq, brd.Occ) & EnemyBishopQueen<white>(brd);
                Bitloop(atkD12) {
                    Square sq = SquareOf(atkD12);
                    CheckBySlider(kingsq, sq, kingban, checkmask);
                }

                map pinnersD12 = Lookup::Bishop_Xray(kingsq, brd.Occ) & EnemyBishopQueen<white>(brd);
                Bitloop(pinnersD12)
                {
                    RegisterPinD12<status>(kingsq, SquareOf(pinnersD12), brd, Movelist::EnPassantTarget);
                }
            }

            if constexpr (status.HasEPPawn)
            {
                RegisterPinEP<status>(kingsq, king, EnemyRookQueen<white>(brd), brd);
            }
        }

        Movestack::Check_Status[depth - 1] = 0xffffffffffffffffull;

        map king_atk = Movestack::Atk_King[depth] & EnemyOrEmpty<status.WhiteMove>(brd) &~kingban;
        if (king_atk == 0) return 0;

        //Calculate Enemy Knight - keep this first
        {
            map knights = Knights<enemy>(brd);
            Bitloop(knights) {
                kingban |= Lookup::Knight(SquareOf(knights));
            }
        }

        //Calculate Check from enemy pawns
        {
            const map pl = Pawn_AttackLeft<enemy>(Pawns<enemy>(brd) & Pawns_NotLeft());
            const map pr = Pawn_AttackRight<enemy>(Pawns<enemy>(brd) & Pawns_NotRight());

            kingban |= (pl | pr);
        }


        //Calculate Enemy Bishop
        {
            map bishops = BishopQueen<enemy>(brd);
            Bitloop(bishops) {
                const Square sq = SquareOf(bishops);
                map atk = Lookup::Bishop(sq, brd.Occ);
                kingban |= atk;
            }
        }

        //Calculate Enemy Rook
        {
            map rooks = RookQueen<enemy>(brd);
            Bitloop(rooks) {
                const Square sq = SquareOf(rooks);
                map atk = Lookup::Rook(sq, brd.Occ);
                kingban |= atk;
            }
        }

        return king_atk &~kingban;
    }

    //Also applies the checkmask
    template<bool IsWhite>
    _ForceInline void Pawn_PruneLeft(map& pawn, const map pinD1D2)
    {
        const map pinned = pawn & Pawn_InvertLeft<IsWhite>(pinD1D2 & Pawns_NotRight()); //You can go left and are pinned
        const map unpinned = pawn & ~pinD1D2;

        pawn = (pinned | unpinned); //You can go left and you and your targetsquare is allowed
    }

    template<bool IsWhite>
    _ForceInline void Pawn_PruneRight(map& pawn, const map pinD1D2)
    {
        const map pinned = pawn & Pawn_InvertRight<IsWhite>(pinD1D2 & Pawns_NotLeft()); //You can go right and are pinned
        const map unpinned= pawn & ~pinD1D2;

        pawn = (pinned | unpinned); //You can go right and you and your targetsquare is allowed
    }


    //THIS CHECKMASK: https://lichess.org/analysis/8/2p5/3p4/KP3k1r/2R1Pp2/6P1/8/8_b_-_e3_0_1
    template<bool IsWhite>
    _ForceInline void Pawn_PruneLeftEP(map& pawn, const map pinD1D2)
    {
        const map pinned = pawn & Pawn_InvertLeft<IsWhite>(pinD1D2 & Pawns_NotRight()); //You can go left and are pinned
        const map unpinned = pawn & ~pinD1D2;

        pawn = (pinned | unpinned);
    }

    template<bool IsWhite>
    _ForceInline void Pawn_PruneRightEP(map& pawn, const map pinD1D2)
    {
        const map pinned = pawn & Pawn_InvertRight<IsWhite>(pinD1D2 & Pawns_NotLeft()); //You can go right and are pinned
        const map unpinned = pawn & ~pinD1D2;

        pawn = (pinned | unpinned); 
    }


    template<bool IsWhite>
    _ForceInline void Pawn_PruneMove(map& pawn, const map pinHV)
    {
        const map pinned = pawn & Pawn_Backward<IsWhite>(pinHV); //You can forward and are pinned by rook/queen in forward direction
        const map unpinned = pawn & ~pinHV;

        pawn = (pinned | unpinned); //You can go forward and you and your targetsquare is allowed
    }

    //This is needed in the case where the forward square is not allowed by the push is ok
    //Example: https://lichess.org/editor?fen=8%2F8%2F8%2F8%2F3K3q%2F8%2F5P2%2F8+w+-+-+0+1
    template<bool IsWhite>
    _ForceInline void Pawn_PruneMove2(map& pawn, const map pinHV)
    {
        const map pinned = pawn & Pawn_Backward2<IsWhite>(pinHV); //You can forward and are pinned by rook/queen in forward direction
        const map unpinned = pawn & ~pinHV;

        pawn = (pinned | unpinned); //You can go forward and you and your targetsquare is allowed
    }

    class VoidClass{};
    template<class BoardStatus status, class Callback_Move, int depth>
    _ForceInline auto _enumerate(const Board& brd, map kingatk, const map kingban, const map checkmask)
    {
        constexpr bool justcount = std::is_same_v<Callback_Move, VoidClass>;
        constexpr bool white = status.WhiteMove;
        const bool noCheck = (checkmask == 0xffffffffffffffffull);
        uint64_t movecnt = 0;

        //All outside variables need to be in local scope as the Callback will change everything on enumeration
        const map pinHV  = RookPin;
        const map pinD12 = BishopPin;
        const map movableSquare = EnemyOrEmpty<white>(brd) & checkmask;
        const map epTarget = EnPassantTarget;

        //Kingmoves
        {
            if constexpr (justcount) {
                movecnt += Bitcount(kingatk);
            }
            else {
                Bitloop (kingatk) 
                { 
                    const Square sq = SquareOf(kingatk);
                    Movestack::Atk_EKing[depth - 1] = Lookup::King(sq);
                    Callback_Move::template Kingmove<status, depth>(brd, King<white>(brd), 1ull << sq);
                }
            }

            //Castling
            //Todo think about how to remove the template if a rook is taken that would have been able to castle
            if constexpr (status.CanCastleLeft()) {
                if (noCheck && status.CanCastleLeft(kingban, brd.Occ, Rooks<white>(brd))) {
                    if constexpr (justcount) { movecnt++; }
                    else {
                        Movestack::Atk_EKing[depth - 1] = Lookup::King(SquareOf(King<white>(brd) << 2));
                        Callback_Move::template KingCastle<status, depth>(brd, (King<white>(brd) | King<white>(brd) << 2), status.Castle_RookswitchL());
                    }
                }
            }
            if constexpr (status.CanCastleRight()) {
                if (noCheck && status.CanCastleRight(kingban, brd.Occ, Rooks<white>(brd))) {
                    if constexpr (justcount) { movecnt++; }
                    else {
                        Movestack::Atk_EKing[depth - 1] = Lookup::King(SquareOf(King<white>(brd) >> 2));
                        Callback_Move::template KingCastle<status, depth>(brd, (King<white>(brd) | King<white>(brd) >> 2), status.Castle_RookswitchR());
                    }
                }
            }
            Movestack::Atk_EKing[depth - 1] = Movestack::Atk_King[depth]; //Default king atk for recursion
        }

        {
            //Horizontal pinned pawns cannot do anything https://lichess.org/editor?fen=3r4%2F8%2F3P4%2F8%2F3K1P1r%2F8%2F8%2F8+w+-+-+0+1
            //Pawns may seem to be able to enpassant/promote but can still be pinned and inside a checkmask
            //Vertical pinned pawns cannot take, but move forward 
            //D12 pinned pawns can take, but never move forward

            const uint64_t pawnsLR = Pawns<white>(brd) & ~pinHV; //These pawns can walk L or R
            const uint64_t pawnsHV = Pawns<white>(brd) & ~pinD12; //These pawns can walk Forward

            //These 4 are basic pawn moves
            uint64_t Lpawns = pawnsLR & Pawn_InvertLeft<white> (Enemy<white>(brd) & Pawns_NotRight() & checkmask);    //Pawns that can take left
            uint64_t Rpawns = pawnsLR & Pawn_InvertRight<white>(Enemy<white>(brd) & Pawns_NotLeft() & checkmask);     //Pawns that can take right
            uint64_t Fpawns = pawnsHV & Pawn_Backward<white>(Empty(brd));	                                          //Pawns that can go forward 
            uint64_t Ppawns = Fpawns & Pawns_FirstRank<white>() & Pawn_Backward2<white>(Empty(brd) & checkmask);      //Pawns that can push
            
            Fpawns &= Pawn_Backward<white>(checkmask); //checkmask moved here to use fpawn for faster Ppawn calc: Pawn on P2 can only push - not move - https://lichess.org/editor?fen=rnbpkbnr%2Fppp3pp%2F4pp2%2Fq7%2F8%2F3P4%2FPPP1PPPP%2FRNBQKBNR+w+KQkq+-+0+1

            //These 4 basic moves get pruned with pin information 
            Pawn_PruneLeft<white>(Lpawns, pinD12);
            Pawn_PruneRight<white>(Rpawns, pinD12);
            Pawn_PruneMove<white>(Fpawns, pinHV);
            Pawn_PruneMove2<white>(Ppawns, pinHV);

            //This is Enpassant
            if constexpr (status.HasEPPawn) {
                //The eppawn must be an enemy since its only ever valid for a single move
                Bit EPLpawn = pawnsLR & Pawns_NotLeft()  & ((epTarget & checkmask) >> 1); //Pawn that can EPTake to the left - overflow will not matter because 'Notleft'
                Bit EPRpawn = pawnsLR & Pawns_NotRight() & ((epTarget & checkmask) << 1);  //Pawn that can EPTake to the right - overflow will not matter because 'NotRight'

                //Special check for pinned EP Take - which is a very special move since even XRay does not see through the 2 pawns on a single rank
                // White will push you cannot EP take: https://lichess.org/editor?fen=8%2F7B%2F8%2F8%2F4p3%2F3k4%2F5P2%2F8+w+-+-+0+1
                // White will push you cannot EP take: https://lichess.org/editor?fen=8%2F8%2F8%2F8%2F1k2p2R%2F8%2F5P2%2F8+w+-+-+0+1
                
                if (EPLpawn | EPRpawn) //Todo: bench if slower or faster
                {
                    Pawn_PruneLeftEP<white>(EPLpawn, pinD12);
                    Pawn_PruneRightEP<white>(EPRpawn, pinD12);

                    if constexpr (justcount) {
                        if (EPLpawn) movecnt++;
                        if (EPRpawn) movecnt++;
                    }
                    else {
                        if (EPLpawn) Callback_Move::template PawnEnpassantTake<status, depth>(brd, EPLpawn, EPLpawn << 1, Pawn_AttackLeft<white>(EPLpawn));
                        if (EPRpawn) Callback_Move::template PawnEnpassantTake<status, depth>(brd, EPRpawn, EPRpawn >> 1, Pawn_AttackRight<white>(EPRpawn));
                    }
                }
            }

            //We have pawns that can move on last rank
            if ((Lpawns | Rpawns | Fpawns) & Pawns_LastRank<white>())
            {
                uint64_t Promote_Left =  Lpawns & Pawns_LastRank<white>();  
                uint64_t Promote_Right = Rpawns & Pawns_LastRank<white>(); 
                uint64_t Promote_Move =  Fpawns & Pawns_LastRank<white>(); 

                uint64_t NoPromote_Left =  Lpawns & ~Pawns_LastRank<white>(); 
                uint64_t NoPromote_Right = Rpawns & ~Pawns_LastRank<white>();
                uint64_t NoPromote_Move =  Fpawns & ~Pawns_LastRank<white>();

                if constexpr (justcount) {
                    movecnt += 4 * Bitcount(Promote_Left);
                    movecnt += 4 * Bitcount(Promote_Right);
                    movecnt += 4 * Bitcount(Promote_Move);

                    movecnt += Bitcount(NoPromote_Left);
                    movecnt += Bitcount(NoPromote_Right);
                    movecnt += Bitcount(NoPromote_Move);
                    movecnt += Bitcount(Ppawns);
                }
                else {
                    while (Promote_Left)    { const Bit pos = PopBit(Promote_Left);     Callback_Move::template Pawnpromote<status, depth>(brd, pos, Pawn_AttackLeft<white>(pos)); }
                    while (Promote_Right)   { const Bit pos = PopBit(Promote_Right);    Callback_Move::template Pawnpromote<status, depth>(brd, pos, Pawn_AttackRight<white>(pos)); }
                    while (Promote_Move)    { const Bit pos = PopBit(Promote_Move);     Callback_Move::template Pawnpromote<status, depth>(brd, pos, Pawn_Forward<white>(pos)); }
                    while (NoPromote_Left)  { const Bit pos = PopBit(NoPromote_Left);   Callback_Move::template Pawnatk<status, depth>(brd, pos, Pawn_AttackLeft<white>(pos)); }
                    while (NoPromote_Right) { const Bit pos = PopBit(NoPromote_Right);  Callback_Move::template Pawnatk<status, depth>(brd, pos, Pawn_AttackRight<white>(pos)); }
                    while (NoPromote_Move)  { const Bit pos = PopBit(NoPromote_Move);   Callback_Move::template Pawnmove<status, depth>(brd, pos, Pawn_Forward<white>(pos)); }
                    while (Ppawns)          { const Bit pos = PopBit(Ppawns);           Callback_Move::template Pawnpush<status, depth>(brd, pos, Pawn_Forward2<white>(pos)); }
                }
            }
            else {
                if constexpr (justcount) {
                    movecnt += Bitcount(Lpawns);
                    movecnt += Bitcount(Rpawns);
                    movecnt += Bitcount(Fpawns);
                    movecnt += Bitcount(Ppawns);
                }
                else {
                    while (Lpawns) { const Bit pos = PopBit(Lpawns); Callback_Move::template Pawnatk<status, depth>(brd, pos,  Pawn_AttackLeft<white>(pos)); }
                    while (Rpawns) { const Bit pos = PopBit(Rpawns); Callback_Move::template Pawnatk<status, depth>(brd, pos,  Pawn_AttackRight<white>(pos)); }
                    while (Fpawns) { const Bit pos = PopBit(Fpawns); Callback_Move::template Pawnmove<status, depth>(brd, pos, Pawn_Forward<white>(pos)); }
                    while (Ppawns) { const Bit pos = PopBit(Ppawns); Callback_Move::template Pawnpush<status, depth>(brd, pos, Pawn_Forward2<white>(pos)); }
                }
            }
        }

        //Knightmoves
        {
            map knights = Knights<white>(brd) &~ (pinHV | pinD12); //A pinned knight cannot move
            Bitloop(knights) {
                const Square sq = SquareOf(knights);
                map move = Lookup::Knight(sq) & movableSquare;

                if constexpr (justcount) {
                    movecnt += Bitcount(move);
                }
                else {
                    while (move) { const Bit to = PopBit(move); Callback_Move::template Knightmove<status, depth>(brd, 1ull << sq, to); }
                }
            }
        }

        //Removing the pinmask outside is faster than having if inside loop
        //Handling rare pinned queens together bishop/rook is faster

        const map queens = Queens<white>(brd);
        //Bishopmoves
        {
            map bishops = Bishops<white>(brd) &~ pinHV; //Non pinned bishops OR diagonal pinned queens

            map bish_pinned = (bishops | queens) & pinD12;
            map bish_nopin = bishops & ~pinD12;
            Bitloop(bish_pinned) {
                const Square sq = SquareOf(bish_pinned);
                map move = Lookup::Bishop(sq, brd.Occ) & movableSquare & pinD12; //A D12 pinned bishop can only move along D12 pinned axis

                if constexpr (justcount) { movecnt += Bitcount(move); }
                else {
                    map pos = 1ull << sq;
                    if (pos & queens) { while (move) { const Bit to = PopBit(move); Callback_Move::template Queenmove<status, depth>(brd, pos, to); } }
                    else {  while (move) { const Bit to = PopBit(move); Callback_Move::template Bishopmove<status, depth>(brd, pos, to); } }
                }
            }
            Bitloop(bish_nopin) {
                const Square sq = SquareOf(bish_nopin);
                map move = Lookup::Bishop(sq, brd.Occ) & movableSquare;

                if constexpr (justcount) { movecnt += Bitcount(move); }
                else {
                    while (move) { const Bit to = PopBit(move); Callback_Move::template Bishopmove<status, depth>(brd, 1ull << sq, to); }
                }
            }
        }

        //Rookmoves
        {
            map rooks = Rooks<white>(brd) & ~pinD12;

            map rook_pinned = (rooks | queens) & pinHV;
            map rook_nopin = rooks & ~pinHV;
            Bitloop(rook_pinned) {
                const Square sq = SquareOf(rook_pinned);
                map move = Lookup::Rook(sq, brd.Occ) & movableSquare & pinHV; //A HV pinned rook can only move along HV pinned axis

                if constexpr (justcount) { movecnt += Bitcount(move); }
                else {
                    map pos = 1ull << sq;
                    if (pos & queens) { while (move) { const Bit to = PopBit(move); Callback_Move::template Queenmove<status, depth>(brd, pos, to); } }
                    else { while (move) { const Bit to = PopBit(move); Callback_Move::template Rookmove<status, depth>(brd, pos, to); } }
                }
            }
            Bitloop(rook_nopin) {
                const Square sq = SquareOf(rook_nopin);
                map move = Lookup::Rook(sq, brd.Occ) & movableSquare; //A HV pinned rook can only move along HV pinned axis

                if constexpr (justcount) { movecnt += Bitcount(move); }
                else {
                    while (move) { const Bit to = PopBit(move); Callback_Move::template Rookmove<status, depth>(brd, 1ull << sq, to); } 
                }
            }
        }
        //Calculate Enemy Queen
        {
            map queens = Queens<white>(brd) & ~(pinHV | pinD12);
            Bitloop(queens) {
                const Square sq = SquareOf(queens);
                map atk = Lookup::Queen(sq, brd.Occ);
                
                map move = atk & movableSquare;

                if constexpr (justcount) {
                    movecnt += Bitcount(move);
                }
                else {
                    while (move) { const Bit to = PopBit(move); Callback_Move::template Queenmove<status, depth>(brd, 1ull << sq, to); }
                }
            }
        }

        if constexpr (justcount) {
            return movecnt;
        }
        else {
            return;
        }
    }

    template<class BoardStatus status, class Callback_Move, int depth>
    _NoInline void EnumerateMoves(Board& brd) //This cannot be forceinline or even inline as its the main recursion entry point
    {
        //Elegant solution for checkmask. Just & with any valid move and it will always be correct. 
        //0 if double check - squares where we can stop a check otherwise. If no check is there - any square is ok (& with ullong max)
        map checkmask = Movestack::Check_Status[depth];
        map kingban = Movestack::Atk_King[depth - 1] = Movestack::Atk_EKing[depth];
        map kingatk = Refresh<status, depth>(brd, kingban, checkmask);

        if (checkmask != 0) {
            _enumerate<status, Callback_Move, depth>(brd, kingatk, kingban, checkmask);
        }
        else {
            Bitloop(kingatk)
            {
                const Square sq = SquareOf(kingatk);
                Movestack::Atk_EKing[depth - 1] = Lookup::King(sq);
                Callback_Move::template Kingmove<status, depth>(brd, King<status.WhiteMove>(brd), 1ull << sq);
            }
        }
    }

    template<class BoardStatus status>
    _ForceInline uint64_t count(Board& brd)
    {
        map checkmask = Movestack::Check_Status[1];
        map kingban = Movestack::Atk_EKing[1];
        map kingatk = Refresh<status, 1>(brd, kingban, checkmask);
         
        if (checkmask != 0)
            return _enumerate<status, VoidClass, 1>(brd, kingatk, kingban, checkmask);  //one check
        else
            return Bitcount(kingatk); //double check
    }



};


