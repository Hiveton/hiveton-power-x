#include "service_emark.h"

#include <stddef.h>

void service_emark_reset(emark_summary_t *summary)
{
    if (summary == NULL)
    {
        return;
    }

    *summary = (emark_summary_t){ 0 };
}

uint8_t service_emark_summarize_identity(const uint32_t *identity_vdos,
                                         uint8_t vdo_count,
                                         emark_summary_t *summary)
{
    if (summary == NULL)
    {
        return 0U;
    }

    service_emark_reset(summary);
    if ((identity_vdos == NULL) || (vdo_count == 0U))
    {
        return 0U;
    }

    /*
     * Cable e-marker data requires cable-directed VDM exchanges (SOP'/SOP'').
     * The current MVP path only observes SOP traffic, so keep the summary empty
     * rather than deriving misleading cable capabilities from partner identity.
     */
    (void)identity_vdos;
    (void)vdo_count;
    return 0U;
}
