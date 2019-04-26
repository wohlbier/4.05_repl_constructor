#include <string>
#include <tuple>
#include <vector>

#include <cilk.h>
#include <memoryweb.h>
extern "C" {
#include <emu_c_utils/layout.h>
}

typedef long Index_t;
typedef long Scalar_t;

/*
 * Overrides default new to always allocate replicated storage for instances
 * of this class. repl_new is intended to be used as a parent class for
 * distributed data structure types.
 */
class repl_new
{
public:
    // Overrides default new to always allocate replicated storage for
    // instances of this class
    static void *
    operator new(std::size_t sz)
    {
        return mw_mallocrepl(sz);
    }
    
    // Overrides default delete to safely free replicated storage
    static void
    operator delete(void * ptr)
    {
        mw_free(ptr);
    }
};

typedef std::vector<std::tuple<Index_t, Scalar_t>> Row_t;
typedef Row_t * pRow_t;
typedef pRow_t * ppRow_t;

class Matrix_t : public repl_new
{
public:
    static Matrix_t * create(Index_t nrows)
    {
        return new Matrix_t(nrows);
    }
    
    Matrix_t() = delete;
    Matrix_t(const Matrix_t &) = delete;
    Matrix_t & operator=(const Matrix_t &) = delete;
    Matrix_t(Matrix_t &&) = delete;
    Matrix_t & operator=(Matrix_t &&) = delete;
    
    Index_t * nodelet_addr(Index_t i)
    {
        // dereferencing causes migrations
        return (Index_t *)(rows_ + i);
    }

    void allocateRows()
    {
#if 1 // this shows ping pong, change 1 to 0 to get rid of ping pong
        // local mallocs on each nodelet
        for (Index_t i = 0; i < nrows_; ++i)
        {
            ppRow_t lrows = rows_;
            cilk_migrate_hint(lrows + i);
            //cilk_migrate_hint(rows_ + i);
            cilk_spawn allocateRow(i);
        }
        cilk_sync;
#else
        Index_t i;
        ppRow_t lrows = rows_;
        i = 0;
        cilk_migrate_hint(lrows + i);
        cilk_spawn allocateRow(i);
        i++;
        cilk_migrate_hint(lrows + i);
        cilk_spawn allocateRow(i);
        i++;
        cilk_migrate_hint(lrows + i);
        cilk_spawn allocateRow(i);
        i++;
        cilk_migrate_hint(lrows + i);
        cilk_spawn allocateRow(i);
        i++;
        cilk_migrate_hint(lrows + i);
        cilk_spawn allocateRow(i);
        i++;
        cilk_migrate_hint(lrows + i);
        cilk_spawn allocateRow(i);
        i++;
        cilk_migrate_hint(lrows + i);
        cilk_spawn allocateRow(i);
        i++;
        cilk_migrate_hint(lrows + i);
        cilk_spawn allocateRow(i);
        cilk_sync;
#endif
    }

private:
    
    Matrix_t(Index_t nrows) : nrows_(nrows)
    {
        nrows_per_nodelet_ = nrows_ + nrows_ % NODELETS();

        rows_ = (ppRow_t)mw_malloc1dlong(nrows_);

        // replicate the class across nodelets
        for (Index_t i = 1; i < NODELETS(); ++i)
        {
            memcpy(mw_get_nth(this, i), mw_get_nth(this, 0), sizeof(*this));
        }

        allocateRows();
    }

    // localalloc a single row
    void allocateRow(Index_t i)
    {
        rows_[i] = new Row_t(); // allocRow must be spawned on correct nlet
    }

    Index_t nrows_;
    Index_t nrows_per_nodelet_;
    ppRow_t rows_;
};

int main(int argc, char* argv[])
{
    starttiming();

    Index_t nrows = 16;

    Matrix_t * A = Matrix_t::create(nrows);

    return 0;
}
