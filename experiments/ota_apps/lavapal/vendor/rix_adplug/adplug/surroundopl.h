/* Shim: rixplay.cpp pulls in "adplug/surroundopl.h" — minimal pass-through. */
#ifndef ADPLUG_SURROUNDOP_SHL
#define ADPLUG_SURROUNDOP_SHL
#include "opl.h"
#include <stdint.h>
class CSurroundopl : public Copl {
public:
    CSurroundopl(int /*rate*/, double /*offset*/, Copl *opl)
        : realOpl(opl) {}
    ~CSurroundopl() { delete realOpl; }
    void write(int reg, int val) override {
        realOpl->setchip(0);
        realOpl->write(reg, val);
        realOpl->setchip(1);
        realOpl->write(reg, val);
        realOpl->setchip(0);
    }
    void init() override { realOpl->init(); }
    void update(short *buf, int samples) override { realOpl->update(buf, samples); }
    Copl *getRealOpl() { return realOpl; }
private:
    Copl *realOpl;
};
#endif
