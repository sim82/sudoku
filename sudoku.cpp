/*
 * Copyright (C) 2011 Simon A. Berger
 * 
 *  This program is free software; you may redistribute it and/or modify its
 *  under the terms of the GNU General Public License as published by the Free
 *  Software Foundation; either version 2 of the License, or (at your option)
 *  any later version.
 *
 *  This program is distributed in the hope that it will be useful, but
 *  WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 *  or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 *  for more details.
 */


#include <array>
#include <vector>
#include <iostream>
#include <fstream>
#include <bitset>
#include <stdexcept>
#include <boost/intrusive/list.hpp>


class board {
    //typedef boost::dynamic_bitset<> bitset;
    
    //typedef small_bitset<unsigned short> bitset;
    typedef std::bitset<9> bitset;
    
    class field {
        bitset *h;
        bitset *v;
        bitset *b;
        
        
        mutable bitset bs;
        mutable bool valid;
        mutable size_t num;
    public:
        
        boost::intrusive::list_member_hook<> hhook;
        boost::intrusive::list_member_hook<> vhook;
        boost::intrusive::list_member_hook<> bhook;
        
        size_t x;
        size_t y;
        

        field( size_t x_, size_t y_, board *board_ ) 
         : 
           h(&board_->geth(x_)),
           v(&board_->getv(y_)),
           b(&board_->getb(x_,y_)),
           valid(false),
           num(-1),
           x(x_),
           y(y_)
        { }
        
        size_t get_num() const {
            
            if( !valid ) {
                get_bs();
                num = bs.count();
            } else if( num == size_t(-1) ) {
                num = bs.count();
            }
            
            return num;
        }
        
        bitset &get_bs() const {
            if( !valid ) {
                bs = (*h);
                bs &= (*v);
                bs &= (*b);
                valid = true;
            }
            
            return bs;
        }
        
        void invalidate() {
            valid = false;
            num = size_t(-1);
            //num = size_t(-1);
        }
        
        bool operator<( const field &other ) const {
            return get_num() < other.get_num();
        }
        
        inline bool is_valid() const {
            return valid;
        }
        
    };
    
    
    std::array<bitset,9> m_vfree;
    std::array<bitset,9> m_hfree;
    std::array<bitset,9> m_bfree;
    
    std::vector<int> m_board;
    
    typedef boost::intrusive::list< field
            , boost::intrusive::member_hook< field, boost::intrusive::list_member_hook<>, &field::hhook> 
            > hlist;
    typedef boost::intrusive::list< field
            , boost::intrusive::member_hook< field, boost::intrusive::list_member_hook<>, &field::vhook> 
            > vlist;
    typedef boost::intrusive::list< field
            , boost::intrusive::member_hook< field, boost::intrusive::list_member_hook<>, &field::bhook> 
            > blist;
    
            
    std::vector<field> m_open;
            
    // these arrays must be declared after m_empty2, otherwise the intrusive hooks might be destructed wile they are still linked (=>assert)
    std::array<hlist,9> m_hfields;
    std::array<vlist,9> m_vfields;
    std::array<blist,9> m_bfields;
    
    
public:
    bitset &getv( size_t y ) {
        assert( y < m_vfree.size() );
        
        return m_vfree[y];
    }

    bitset &geth( size_t x ) {
            
        assert( x < m_hfree.size() );
        
        return m_hfree[x];
    }
    
    bitset &getb( size_t x, size_t y ) {
        assert( x < 9 );
        assert( y < 9 );
        size_t b = get_block( x, y );
        
        return m_bfree[b];
    }
    
    std::vector<int>::iterator get_field( size_t x, size_t y ) {
        return m_board.begin() + get_addr( x, y );
    }
    
    size_t get_block( size_t x, size_t y ) {
        assert( x < 9 );
        assert( y < 9 );
        
        
        return (y / 3) * 3 + (x / 3);  
    }

    size_t get_addr( size_t x, size_t y ) {
        assert( x < 9 );
        assert( y < 9 );
        
        
        return y * 9 + x;
        
    }
    
    
    class manipulate {
        board *m_board;
        
        const size_t x;
        const size_t y;
        
        bitset &h;
        bitset &v;
        bitset &b;
        
        std::vector<int>::iterator field;
        
        int num;
        bool m_commit;
    public:
        manipulate( board *board_, size_t x_, size_t y_, int num_ )      
          : m_board(board_),
            x(x_),
            y(y_),
            h(board_->geth(x)),
            v(board_->getv(y)),
            b(board_->getb(x,y)),
            field( board_->get_field(x,y)),
            num(num_),
            m_commit(false)
        {
            assert( num >= 0 && num <= 8 );
            
            assert( h[num] );
            assert( v[num] );
            assert( b[num] );
            //size_t addr = board_->get_addr(x, y);
            assert( *field == -1 );
            
            h[num] = false;
            v[num] = false;
            b[num] = false;
            
            *field = num;
            m_board->invalidate(x,y);
        }
        ~manipulate() {
                
            if( !m_commit ) {
                h[num] = true;
                v[num] = true;
                b[num] = true;
                
                *field = -1;
                
                m_board->invalidate(x,y);
            }
        }
        
        void commit() {
            m_commit = true;
        }
    };
    
    
    board() : m_board(9 * 9, -1 )
    
    {

        
        for( int i = 0; i < 9; i++ ) {
            m_hfree[i].flip();
            m_vfree[i].flip();
            m_bfree[i].flip();
            
//             std::cout << m_hfree[i].count() << "\n";
        }   
        
    }
    
    void link( field *f ) {
        hlist &h = m_hfields[f->x];
        vlist &v = m_vfields[f->y];
        blist &b = m_bfields[get_block(f->x, f->y)];
        
        h.push_back( *f );
        v.push_back( *f );
        b.push_back( *f );
        
    }
    
     void unlink( field *f ) {
         hlist &h = m_hfields[f->x];
         vlist &v = m_vfields[f->y];
         blist &b = m_bfields[get_block(f->x, f->y)];
         
         h.erase( h.iterator_to(*f) );
         v.erase( v.iterator_to(*f) );
         b.erase( b.iterator_to(*f) );
     }
    
    void invalidate( size_t x, size_t y ) {
        hlist &h = m_hfields[x];
        vlist &v = m_vfields[y];
        blist &b = m_bfields[get_block(x, y)];
        
        auto inval = [x,y,this]( field &f ) {
            f.invalidate();
        };
        std::for_each( h.begin(), h.end(), inval );
        std::for_each( v.begin(), v.end(), inval );
        std::for_each( b.begin(), b.end(), inval );
    }
        
    void init( const std::string &s ) {
        
        auto sit = s.begin();
        m_open.reserve(81);
        for( int i = 0; i < 9; ++i ) {
            for( int j = 0; j < 9; ++j, ++sit ) {
             
                if( sit == s.end() ) {
                 
                    std::cerr << "line end too early: " << i << " " << j << "\n";
                    throw std::runtime_error( "bailing out" );
                }
                
                assert( sit != s.end() );
                
                if( *sit >= '1' && *sit <= '9' ) {
                    
                    
                    int num = *sit - '1';
                    
                    if( num < 0 || num > 8 ) {
                     
                        std::cerr << "bad number in input: " << *sit << "\n";
                        throw std::runtime_error( "bailing out" );
                    }
                    
                    if( !geth(j)[num] || !getv(i)[num] || !getb(j,i)[num] ) {
                        std::cerr << "inconsistent input: " << j << " " << i << " " << *sit << "\n";
                        throw std::runtime_error( "bailing out" );
                    }
                    
                    manipulate m(this, j, i, num );
                    m.commit();
                } else {
                    
                    
                    m_open.push_back( field( j, i, this ) );
                    
                    link( &m_open.back() );
                }
                
                
            }
        }
        std::for_each(m_open.begin(), m_open.end(), []( field &f ) {f.invalidate();} );
    }
        
    void print() {
        for( int i = 0; i < 9; ++i ) {
            for( int j = 0; j < 9; ++j ) {
             
                auto field = get_field(j, i);
                
                if( *field != -1 ) {
                 
                    std::cout << *field + 1 << " ";
                } else {
                        
                    std::cout << "  ";
                }
                
            }
            std::cout << "\n";
    
        }
    }

    
    bool solve() {
     
        if( m_open.empty() ) {
            return true;
        }
        
        auto fit = std::min_element(m_open.begin(), m_open.end());
        
        
        unlink( &(*fit) );
        field field = std::move(*fit); // DON'T use reference! (what would 'auto' do here?)
        
        if( m_open.size() > 1 ) {
            if( fit != m_open.end() - 1 ) {
                unlink( &m_open.back() );
                *fit = std::move(m_open.back() );
                link( &(*fit) );
            }
        }
        
        
        m_open.pop_back();
        
        bitset &p = field.get_bs();
        size_t num = p._Find_first();
        
        while( num != p.size() ) {
            manipulate m( this, field.x, field.y, num );
            
            bool ret = solve();
            
            if( ret ) {
                m.commit();
                return true;
            }
            
            num = p._Find_next(num);
        }
        
        m_open.push_back(field);
        link( &m_open.back() );
        return false;
    }
    
};


int main( int argc, char *argv[] ) {
    
    
    
    if( argc != 2 ) {
        std::cerr << "missing file name\n";
        throw std::runtime_error( "bailing out" );
    }
    std::ifstream is( argv[1] );
    
    if( !is.good() ) {
        std::cerr << "cannot read file: " << argv[1] << "\n";
        throw std::runtime_error( "bailing out" );
    }
    while( !is.eof() ) {
        
        
        std::string line;
        std::getline( is, line ); 
        
        if( line.size() < 81 ) {
            std::cout << "end\n";
            break;
        }
        
        board s;
        s.init(line);
        bool ret = s.solve();
    
        std::cout << "ret: " << ret << "\n";
    
        s.print();    
    }
//     s.init( is );
    
//     s.print();
    
    
}
// template<typename block_t>
// class small_bitset {
//     const static size_t max_bits = sizeof(block_t) * 8;
//     
//     block_t block;
// public:
//     
//     class reference {
//         
//         block_t m_block;
//         const block_t m_mask;
//         
//         void do_set() { m_block |= m_mask; }
//         void do_reset() { m_block &= ~m_mask; }
//         void do_flip() { m_block ^= m_mask; }
//         void do_assign(bool x) { x? do_set() : do_reset(); }
//         
//         
//         
//     public:
//         reference( block_t &block_, size_t pos_ ) : m_block(block_), m_mask( 1 << pos_ ) {}
//         
//         operator bool() const { return (m_block & m_mask) != 0; }
//         bool operator~() const { return (m_block & m_mask) == 0; }
// 
//         reference& flip() { do_flip(); return *this; }
// 
//         reference& operator=(bool x)               { do_assign(x);   return *this; } // for b[i] = x
//         reference& operator=(const reference& rhs) { do_assign(rhs); return *this; } // for b[i] = b[j]
// 
//         reference& operator|=(bool x) { if  (x) do_set();   return *this; }
//         reference& operator&=(bool x) { if (!x) do_reset(); return *this; }
//         reference& operator^=(bool x) { if  (x) do_flip();  return *this; }
//         reference& operator-=(bool x) { if  (x) do_reset(); return *this; }
//         
//         
//     };
//     
//     size_t npos;
//     
//     
//     small_bitset(size_t npos_) : block(0), npos(npos_) {
//         assert( npos <= max_bits );
//     }
//     
//     size_t find_first() {
//         block_t t = 1;
//         
//         for( size_t i = 0; i < npos; ++i, t<<=1 ) {
//         
//             if( (block & t) != 0 ) {
//                 return i;
//             }
//         }
//         
//         return npos;
//         
//     }
//     
//     size_t find_next( size_t first ) {
//         if( first >= npos ) {
//             return npos;
//         } else {
//             block_t t = 1 << first + 1;
//             
//             for( size_t i = first + 1; i < npos; ++i, t<<=1 ) {
//         
//                 if( (block & t) != 0 ) {
//                     return i;
//                 }
//             }
//             
//             
//             return npos;
//         }
//       
//         
//         
//         
//     }
//     
//     bool test( size_t pos ) {
//         assert( pos > npos );
//         block_t t = 1 << pos;
//         
//         return (block & t) != 0;
//     }
//     
//     reference operator[](size_t pos) {
//         return reference(block, pos);
//     }
//     bool operator[](size_t pos) const { return test(pos); }
//     
//     void flip() {
//         block ^= block_t(-1);
//     }
//     
// };