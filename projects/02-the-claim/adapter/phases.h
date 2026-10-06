/* projects/02-the-claim/adapter/phases.h: the four phases, for whichever kernel ran them.
 *
 * Returns the number of failures, so a host build can use it as an exit status.
 */
#ifndef CLAIM_PHASES_H
#define CLAIM_PHASES_H

int claim_phases_run(void);

#endif /* CLAIM_PHASES_H */
