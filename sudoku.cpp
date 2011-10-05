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

#include <vector>
#include <boost/dynamic_bitset.hpp>
#include <iostream>
#include <fstream>

class board {
    std::vector<boost::dynamic_bitset<> > m_vfree;
    std::vector<boost::dynamic_bitset<> > m_hfree;
    std::vector<boost::dynamic_bitset<> > m_bfree;
    
    std::vector<int> m_board;
    
    std::vector<std::pair<size_t, size_t> > m_empty;
    
public:
    boost::dynamic_bitset<> &getv( size_t y ) {
        assert( y < m_vfree.size() );
        
        return m_vfree[y];
    }

    boost::dynamic_bitset<> &geth( size_t x ) {
            
        assert( x < m_hfree.size() );
        
        return m_hfree[x];
    }
    
    boost::dynamic_bitset<> &getb( size_t x, size_t y ) {
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
        const size_t x;
        const size_t y;
        
        boost::dynamic_bitset<> &h;
        boost::dynamic_bitset<> &v;
        boost::dynamic_bitset<> &b;
        
        std::vector<int>::iterator field;
        
        int num;
        bool m_commit;
    public:
        manipulate( board *board_, size_t x_, size_t y_, int num_ )      
          : x(x_),
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
            
        }
        ~manipulate() {
                
            if( !m_commit ) {
                h[num] = true;
                v[num] = true;
                b[num] = true;
                
                *field = -1;
            }
        }
        
        void commit() {
            m_commit = true;
        }
    };
    
    
    board() : m_board(9 * 9, -1 ) 
    
    {
        boost::dynamic_bitset<> tmp(9);
        tmp.flip();
       
        
        for( int i = 0; i < 9; i++ ) {
            m_vfree.push_back(tmp);
            m_hfree.push_back(tmp);
            m_bfree.push_back(tmp);
       
            
            std::cout << m_hfree[i].count() << "\n";
        }   
        
    }
    
    void init( std::istream &is ) {
        
        for( int i = 0; i < 9; ++i ) {
            std::string line;
            std::getline( is, line );
            
            auto lit = line.begin();
            
            for( int j = 0; j < 9; ++j, ++lit ) {
             
                if( lit == line.end() ) {
                 
                    std::cerr << "line end too early: " << i << " " << j << "\n";
                    throw std::runtime_error( "bailing out" );
                }
                
                assert( lit != line.end() );
                
                if( *lit != ' ' ) {
                    assert( *lit >= '1' && *lit <= '9' );    
                    
                    int num = *lit - '1';
                    
                    if( num < 0 || num > 8 ) {
                     
                        std::cerr << "bad number in input: " << *lit << "\n";
                        throw std::runtime_error( "bailing out" );
                    }
                    
                    if( !geth(j)[num] || !getv(i)[num] || !getb(j,i)[num] ) {
                        std::cerr << "inconsistent input: " << j << " " << i << " " << *lit << "\n";
                        throw std::runtime_error( "bailing out" );
                    }
                    
                    manipulate m(this, j, i, num );
                    m.commit();
                } else {
                    m_empty.push_back(std::make_pair( size_t(j), size_t(i)) );
                }
                
                
            }
        }
        
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
     
        if( m_empty.empty() ) {
            return true;
        }
            
        auto field = m_empty.back();
        m_empty.pop_back();
        
       
        boost::dynamic_bitset<> p = geth(field.first);
        
        p &= getv(field.second);
        p &= getb(field.first, field.second);
        
        //assert(p.count()>0);
        
        size_t num = p.find_first();
        while( num != p.npos ) {
            manipulate m( this, field.first, field.second, num );
            
            bool ret = solve();
            
            if( ret ) {
             
                m.commit();
                return true;
            }
            
            num = p.find_next(num);
        }
        
        m_empty.push_back(field);
        
        return false;
    }
    
};


int main( int argc, char *argv[] ) {
    
    
    board s;
    assert( argc == 2 );
    std::ifstream is( argv[1] );
    
    assert( is.good() );
    
    s.init( is );
    
    s.print();
    
    bool ret = s.solve();
    
    std::cout << "ret: " << ret << "\n";
    
    s.print();
}