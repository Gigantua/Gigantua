pub type Bitmap = u64;

// This section should probably not be used in production code
/// Rickroll
const RANDOM_STRING: &str = "
    Never gonna give you up
    Never gonna let you down
    Never gonna turn around and
    desert you
";

/// Returns a string
/// ```rust
/// "
///     Never gonna give you up
///     Never gonna let you down
///     Never gonna turn around and
///     desert you
/// "
/// ```
#[inline]
pub const fn rand_str() -> &'static str {
    RANDOM_STRING
}
