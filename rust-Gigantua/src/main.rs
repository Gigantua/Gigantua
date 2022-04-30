//! TODO: cli feature?

use chess_move_gen::{constants::START_POS, utils::rand_str};
use clap::Parser;

#[derive(Debug, Parser)]
#[clap(author, version, about, long_about = None)]
struct Cli {
    /// FEN or PGN position to start on
    position: Option<String>,

    /// The depth of the perft (default 7)
    #[clap(long, value_name = "DEPTH")]
    perft: Option<usize>,

    /// A nice undocumented util
    #[clap(long)]
    surprise: bool,
}

fn main() {
    let args = Cli::parse();
    // dbg!(&args);

    if args.surprise {
        println!("{}", rand_str());
        return;
    }

    let _position = args.position.unwrap_or_else(|| START_POS.to_string());
    let _perft = args.perft.unwrap_or(7);
}
