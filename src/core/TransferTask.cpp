#include "TransferTask.h"

static int registerMetaTypes()
{
    qRegisterMetaType<TransferItem>("TransferItem");
    qRegisterMetaType<TransferStatus>("TransferStatus");
    qRegisterMetaType<TransferMode>("TransferMode");
    return 0;
}
static int _metaTypesRegistered = registerMetaTypes();
