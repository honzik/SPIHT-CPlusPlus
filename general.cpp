#include "general.h"

// aux function to compute log2
wUnit log2(wUnit x)
{
  static const double xxx = 1.0/log(2.0);
  return log(x)*xxx;
}

// simple round... at least I suppose that's how round works heh? :)
wUnit round(wUnit x) {
	return (x < floor(x) + 0.5) ? floor(x) : ceil(x);
}
