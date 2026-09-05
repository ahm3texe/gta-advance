/* Two IDs are compatible when equal or when both belong to the special ID group.
 * Group membership is recovered from the two ROM switch tables.
 */
#include "gba_types.h"

u32 IdsCompatible(u32 first, u32 second)
{
    if (first == second)
        goto compatible;
    switch (first) {
    case 20:
    case 38:
    case 39:
    case 45:
    case 46:
    case 51:
    case 55:
    case 66:
    case 67:
    case 68:
    case 69:
    case 70:
    case 71:
    case 72:
    case 73:
    case 74:
    case 75:
    case 76:
    case 77:
        switch (second) {
    case 20:
    case 38:
    case 39:
    case 45:
    case 46:
    case 51:
    case 55:
    case 66:
    case 67:
    case 68:
    case 69:
    case 70:
    case 71:
    case 72:
    case 73:
    case 74:
    case 75:
    case 76:
    case 77:
            goto compatible;
        }
    }
    return 0;
compatible:
    return 1;
}
