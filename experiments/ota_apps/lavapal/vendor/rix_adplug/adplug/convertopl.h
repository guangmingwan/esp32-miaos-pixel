/* Shim: rixplay.cpp pulls in "adplug/convertopl.h" — mono/stereo OPL passthrough. */
#ifndef ADPLUG_CONVERTOPL_H_SHIM
#define ADPLUG_CONVERTOPL_H_SHIM
#include "opl.h"
class CConvertopl : public Copl {
public:
    CConvertopl(Copl *opl, bool /*bit16*/, bool /*stereo*/)
        : realOpl(opl) {}
    ~CConvertopl() { delete realOpl; }
    void write(int reg, int val) override { realOpl->write(reg, val); }
    void init() override { realOpl->init(); }
    void update(short *buf, int samples) override { realOpl->update(buf, samples); }
private:
    Copl *realOpl;
};
#endif
