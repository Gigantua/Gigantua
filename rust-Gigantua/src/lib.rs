#![allow(dead_code)]
#![feature(const_ptr_read)]

pub mod movelist;
pub mod movemap;
pub mod utils;

pub mod constants {
    pub const START_POS: &str = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
}

struct MoveReceiver {
    nodes: u64,
}

impl MoveReceiver {
    const fn new() -> Self {
        MoveReceiver { nodes: 0 }
        // TODO 1: MoveList
    }
}

// Tests are moved to tests.rs
