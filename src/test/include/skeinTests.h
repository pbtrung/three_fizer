#ifndef SKEINTESTS_H
#define SKEINTESTS_H

#include "debug.h"
#include "threefizer/skeinApi.h"
#include <assert.h>
#include <stdint.h>

/*
* K3rb3ros 10.12.2014
* These values were taken from a freshly initialized SkeinCtx on my test machine
*/

const static uint64_t ExpectedState256[Skein256/64] = { 18202890402666165321L, 3443677322885453875L, 12915131351309911055L, 7662005193972177513L };

const static uint64_t ExpectedState512[Skein512/64] = { 5261240102383538638L, 978932832955457283L, 10363226125605772238L, 11107378794354519217L, 6752626034097301424L, 16915020251879818228L, 11029617608758768931L, 12544957130904423475L };

const static uint64_t ExpectedState1024[Skein1024/64] = { 15389884076360409941L, 1564408309651595276L, 5872946452714669296L, 269443931446226863L, 2066808069366720664L, 7949147512366979231L, 8638675932900051674L, 13915592562466140323L, 15479235830881622709L, 7681746768227630605L, 10539485315553235762L, 1882256010534388436L, 690868493690407864L, 7310148287346284186L, 7060852991742072730L, 2152812356051330361L };

const static uint64_t Skein256NullHash[Skein256/64] = { 8724517391939661071L, 13370976164479673572L, 17406185901391511763L, 5767528745175948617L };

const static uint64_t Skein512NullHash[Skein512/64] = { 16674971110164789043L, 16271238091670380495L, 13753278159638688213L, 15640160688208848212L, 6426357076001091602L, 7868118233575035598L, 13409637217154661878L, 15823918759942544260L };

const static uint64_t Skein1024NullHash[Skein1024/64] = { 5767348195165280385L, 13313145644582907738L, 15445495210649757637L, 784299716472470282L, 6258770670021293714L, 8003395179891192994L, 5536180265455278323L, 3682281098039611423L, 950856614975086330L, 11517112629637956673L, 4305894531452524800L, 16828010516203810237L, 16645466546792041892L, 8220960308264854982L, 1879382561755265108L, 13388187221246470230L };

const static uint64_t TestMsg[2] = { 5077073466687252581L , 2327889596035104768L };

/*
* K3rb3ros 10.19.2014
* These values were taken from Skein digests on my test machine
*/

const static uint64_t ExpectedDigest256[Skein256/64] = { 5913355494111548831L, 14545365143941776789L, 3101499150589447885L, 14330232381075572585L };

const static uint64_t ExpectedDigest512[Skein512/64] = { 17820824062134001381L, 1920143778435347109L, 11915273695160806815L, 4539088631920594701L, 14090049249442769627L, 14822485233854683647L, 11956900317171193513L, 17778993175431006280L };

const static uint64_t ExpectedDigest1024[Skein1024/64] = { 8465303633664212295L, 4846274381220427229L, 12614322184485712757L, 7503119550539696887L, 7005327482526261793L, 13756209870218924162L, 476568214109261173L, 1867171129410923627L, 16099196659237491457L, 589659456053222171L, 10255270865036076450L, 17448666692730491597L, 9165797820039646890L, 10872441060414999887L, 9011713385788166076L, 3434631984923120672L };

void runSkeinTests();

#endif
