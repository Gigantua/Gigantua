#pragma once
#include "Chess_Base.hpp"


static constexpr uint64_t File1 = 0b1000000010000000100000001000000010000000100000001000000010000000ul;
static constexpr uint64_t File8 = 0b0000000100000001000000010000000100000001000000010000000100000001ul;
static constexpr uint64_t Rank2 = 0b0000000000000000000000000000000000000000000000001111111100000000ul;
static constexpr uint64_t Rank7 = 0b0000000011111111000000000000000000000000000000000000000000000000ul;
static constexpr uint64_t RankMid = 0x0000FFFFFFFF0000;
static constexpr uint64_t Rank_18 = 0xFF000000000000FF;

_Compiletime map Pawns_NotLeft() {
	return ~File1;
}

_Compiletime map Pawns_NotRight() {
	return ~File8;
}

template<bool IsWhite>
_Compiletime map Pawn_Forward(map mask) {
	if constexpr (IsWhite) return mask << 8;
	else return mask >> 8;
}

template<bool IsWhite>
_Compiletime map Pawn_Forward2(map mask) {
	if constexpr (IsWhite) return mask << 16;
	else return mask >> 16;
}

template<bool IsWhite>
_Compiletime map Pawn_Backward(map mask) {
	return Pawn_Forward<!IsWhite>(mask);
}

template<bool IsWhite>
_Compiletime map Pawn_Backward2(map mask) {
	return Pawn_Forward2<!IsWhite>(mask);
}

template<bool IsWhite>
_Compiletime map Pawn_AttackLeft(map mask) {
	if constexpr (IsWhite) return mask << 9;
	else return mask >> 7;
}

template<bool IsWhite>
_Compiletime map Pawn_AttackRight(map mask) {
	if constexpr (IsWhite) return mask << 7;
	else return mask >> 9;
}

template<bool IsWhite>
_Compiletime map Pawn_InvertLeft(map mask) {
	return Pawn_AttackRight<!IsWhite>(mask);
}

template<bool IsWhite>
_Compiletime map Pawn_InvertRight(map mask) {
	return Pawn_AttackLeft<!IsWhite>(mask);
}

template<bool IsWhite>
constexpr map Pawns_FirstRank() {
	if constexpr (IsWhite) return Rank2;
	else return Rank7;
}

template<bool IsWhite>
constexpr map Pawns_LastRank() {
	if constexpr (IsWhite) return Rank7;
	else return Rank2;
}

template<bool IsWhite>
_Compiletime Bit King(const Board& brd) {
	if constexpr (IsWhite) return brd.WKing;
	else return brd.BKing;
}

template<bool IsWhite>
_Compiletime Bit EnemyKing(const Board& brd)
{
	if constexpr (IsWhite) return brd.BKing;
	else return brd.WKing;
}

template<bool IsWhite>
_Compiletime map Pawns(const Board& brd) {
	if constexpr (IsWhite) return brd.WPawn;
	else return brd.BPawn;
}

template<bool IsWhite>
_Compiletime map OwnColor(const Board& brd)
{
	if constexpr (IsWhite) return brd.White;
	return brd.Black;
}
template<bool IsWhite>
_Compiletime map Enemy(const Board& brd)
{
	if constexpr (IsWhite) return brd.Black;
	return brd.White;
}

template<bool IsWhite>
_Compiletime map EnemyRookQueen(const Board& brd)
{
	if constexpr (IsWhite) return brd.BRook | brd.BQueen;
	return brd.WRook | brd.WQueen;
}

template<bool IsWhite>
_Compiletime map RookQueen(const Board& brd)
{
	if constexpr (IsWhite) return brd.WRook | brd.WQueen;
	return brd.BRook | brd.BQueen;
}

template<bool IsWhite>
_Compiletime map EnemyBishopQueen(const Board& brd)
{
	if constexpr (IsWhite) return brd.BBishop | brd.BQueen;
	return brd.WBishop | brd.WQueen;
}

template<bool IsWhite>
_Compiletime map BishopQueen(const Board& brd)
{
	if constexpr (IsWhite) return brd.WBishop | brd.WQueen;
	return brd.BBishop | brd.BQueen;
}

template<bool IsWhite>
_Compiletime map EnemyOrEmpty(const Board& brd)
{
	if constexpr (IsWhite) return ~brd.White;
	return ~brd.Black;
}

_Compiletime map Empty(const Board& brd)
{
	return ~brd.Occ;
}


template<bool IsWhite>
_Compiletime map Knights(const Board& brd)
{
	if constexpr (IsWhite) return brd.WKnight;
	return brd.BKnight;
}

template<bool IsWhite>
_Compiletime map Rooks(const Board& brd)
{
	if constexpr (IsWhite) return brd.WRook;
	return brd.BRook;
}

template<bool IsWhite>
_Compiletime map Bishops(const Board& brd)
{
	if constexpr (IsWhite) return brd.WBishop;
	return brd.BBishop;
}


template<bool IsWhite>
_Compiletime map Queens(const Board& brd)
{
	if constexpr (IsWhite) return brd.WQueen;
	return brd.BQueen;
}

