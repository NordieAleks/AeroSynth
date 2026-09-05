#include "SynthEngine.h"

// SynthEngine, SynthVoice, and the oscillator/envelope/filter classes are
// implemented inline in their headers for the compiler to freely inline
// across the hot audio-processing call chain (oscillator -> filter -> mix).
// This translation unit exists so the class still has a single definition
// point for tooling (symbol export, unity-build boundaries) even though
// there's no additional out-of-line code to add here.
