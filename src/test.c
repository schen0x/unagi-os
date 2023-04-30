#include "test.h"
#include "util/dlist.h"
#include "util/kutil.h"
#include "util/fifo.h"

bool test_all()
{
	if(!test_dlist())
		return false;

	if (!test_kutil())
		return false;
	if (!test_fifo8())
		return false;
	return true;
}

