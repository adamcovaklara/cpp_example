//
//  main.cpp
//  zapocet
//
//  Created by MacBook on 08/05/2021.
//  Copyright © 2021 Klara Pacalova. All rights reserved.
//

#ifndef __PROGTEST__
#include <iostream>
#include <iomanip>
#include <sstream>
#include <cstdint>
#include <cassert>
#include <cstdlib>
#include <cstdio>
#include <vector>
#include <list>
#include <set>
#include <map>
#include <queue>
#include <stack>
#include <deque>
#include <algorithm>
#include <memory>
#include <functional>
#include <stdexcept>
using namespace std;

enum class ERel
{
  REL_NONE,
  REL_FATHER,
  REL_MOTHER,
  REL_SON,
  REL_DAUGHTER
};
#endif /* __PROGTEST__ */

inline bool stringMatch(const std::string& prefix, const std::string& str)
{
    return !str.compare(0, prefix.size(), prefix);
}

class CRegister;

class CBase
{
  public:
    using ptr = std::shared_ptr<CBase>;
    
    virtual ~CBase() = default;
    
    // constructor
    CBase(int64_t ID, const std::string& fullName, CRegister* reg = nullptr) : m_ID(ID), name(fullName), reg(reg) {
    }
    
    int64_t GetID () const
    {
        return m_ID;
    }
    
    const std::string& GetName() const
    {
        return name;
    }
    
    // operator <<
    friend std::ostream& operator<<(std::ostream& os, const CBase& base)
    {
        return base.print(os);
    }
    
      void PrintDescendants (std::ostream& os, const std::string& prefix = "") const
      {
          os << *this << std::endl;
          for (std::size_t childId = 0; childId < m_Descendants.size(); ++childId)
          {
              bool isLast(childId == m_Descendants.size() - 1);
              os << prefix << (isLast ? " \\- " : " +- ");
              m_Descendants[childId]->PrintDescendants(os, prefix + (isLast ? "   " : " | "));
          }
      }

    void addDescendant(ptr child) {
        if (!child) { return; }
        m_Descendants.push_back(child);
        
    }
    
    virtual bool isMatch(const std::string& prefix) const
    {
        return stringMatch(prefix, GetName());
    }
    
    friend class CRegister;
    
    virtual ptr clone(CRegister * reg) const = 0;
protected:
    virtual inline std::ostream& print(std::ostream& os) const
    {
        return os << m_ID << ": " << name << intermezzo()
                  << " (" << type() << ")";
    }

    virtual std::string intermezzo() const { return ""; };
    virtual std::string type() const = 0;
    
    int64_t m_ID;
    std::string name;
    std::vector<ptr> m_Descendants;
    // needed for tree bottom-top traversal
    ptr mother, father;
    CRegister * reg = nullptr;
};

class CMan : public CBase
{
  public:
    virtual ~CMan() = default;
    // constructor
    CMan(int64_t ID, const std::string& name) : CBase(ID, name) {}

  private:
    inline CBase::ptr clone(CRegister *) const override
    {
        return std::make_shared<CMan>(m_ID, GetName());
    }
    
    inline std::string type() const override { return "man"; }
};


class CRegister
{
  public:
    bool Add (CBase::ptr person, CBase::ptr father, CBase::ptr mother)
    {
        if (!person) { return false; }
       
        std::int64_t k = findInternal(person->GetID());
        if ((std::size_t)k < peeps.size() && peeps[k]->GetID() == person->GetID()) {
            return false;
        }
        
        auto it = std::begin(peeps) + k;
        CBase::ptr child = person->clone(this);
        peeps.insert(it, child);
        if (father) {
            father->addDescendant(child);
            child->father = father;
        }
        if (mother) {
            mother->addDescendant(child);
            child->mother = mother;
        }
        
        addInternal(child->GetName(), child);
        
        return true;
    }
    
    CBase::ptr FindByID (int64_t id) const
    {
        std::int64_t k = findInternal(id);
        return ((std::size_t)k < peeps.size() && peeps[k]->GetID() == id) ? peeps[k] : nullptr;
    }
    
    vector<CBase::ptr> FindByName (const std::string& prefix) const
    {
        vector<CBase::ptr> found;
        auto it = std::lower_bound(std::begin(peepsByName), std::end(peepsByName), prefix,
                                   [](const auto& p, const std::string& val)
        {
            return p.first < val;
        });
       
        for (; it != std::end(peepsByName) && stringMatch(prefix, it->first); ++it)
        {
            found.push_back(it->second);
        }
        
        std::sort(std::begin(found), std::end(found),[](const auto& lhs, const auto& rhs)
        {
            return lhs->GetID() < rhs->GetID();
        });
        
        auto last = std::unique(std::begin(found), std::end(found),[](const auto& lhs, const auto& rhs)
        {
            return lhs->GetID() == rhs->GetID();
        });
        
        found.erase(last, std::end(found));
        
        return found;
    }
    
    auto FindRelatives (int64_t id1, int64_t id2) const
    {
        using qtype = std::list<std::pair<CBase::ptr, ERel>>;

        auto cmp([](const qtype& lhs, const qtype& rhs) {
            // prefer shorter relationship path
            return lhs.size() > rhs.size();
        });
        
        std::priority_queue<qtype, std::vector<qtype>, decltype(cmp)> bfs(cmp);
        
        CBase::ptr first { FindByID(id1) };
        CBase::ptr second { FindByID(id2) };
        if (!first || !second || id1 == id2) {
            throw std::invalid_argument("Error.");
        }
        
        std::set<int64_t> processed;
        processed.insert(first->GetID());
        bfs.emplace(qtype({std::make_pair(first, ERel::REL_NONE)}));
        
        auto isDone([&processed](CBase::ptr candidate) {
            return !candidate ||
                    processed.find(candidate->GetID()) != std::end(processed);
        });
        
        auto addQueue([&](qtype currentList, CBase::ptr p, ERel rel)
        {
            currentList.emplace_back(std::piecewise_construct,
                                     std::forward_as_tuple(p),
                                     std::forward_as_tuple(rel));
            bfs.emplace(currentList);
            processed.insert(p->GetID());
        });
        
        while(!bfs.empty())
        {
            qtype top = bfs.top();
            bfs.pop();
            
            CBase::ptr last = top.back().first;
            if (last->GetID() == id2) {
                // erase first element
                top.erase(std::begin(top));
                return top;
            }
            
            for (CBase::ptr child : last->m_Descendants) {
                if (isDone(child)) { continue; }
                
                addQueue(top, child, child->type() == "man" ? ERel::REL_SON
                                                            : ERel::REL_DAUGHTER);
            }
            // process parents
            if (!isDone(last->father)) {
                addQueue(top, last->father, ERel::REL_FATHER);
            }
            
            if (!isDone(last->mother)) {
                addQueue(top, last->mother, ERel::REL_MOTHER);
            }
        }
        
        // no relationship
        return qtype();
    }
    
    void addInternal(const std::string& name, CBase::ptr ptr)
    {//comp(*it, value)
        auto it = std::lower_bound(std::begin(peepsByName), std::end(peepsByName), std::pair<std::string, std::int64_t>(name, ptr->GetID()),
                                   [](const auto& p, const std::pair<std::string, std::int64_t>& val)
        {
            return std::make_pair(p.first, p.second->GetID()) < val;
        });
        if (it != std::end(peepsByName) &&
            it->second->GetID() == ptr->GetID()
            && it->first == name) {
            return;
        }
        
        peepsByName.emplace(it, name, ptr);
    }
private:
    std::int64_t findInternal(int64_t id) const {
        auto it = std::lower_bound(std::begin(peeps), std::end(peeps), id, [](const CBase::ptr& p, int64_t val)
        {
            return p->GetID() < val;
        });
        
        return std::distance(std::begin(peeps), it);
    }
    
    std::vector<CBase::ptr> peeps;
    
    std::vector<std::pair<std::string, CBase::ptr>> peepsByName;
};


class CWoman : public CBase
{
public:
    virtual ~CWoman() = default;
    CWoman(int64_t ID, const std::string& name, const std::vector<std::string>& prev = {}, CRegister * reg = nullptr) : CBase(ID, name, reg), m_prev_names(prev) {}

    void Wedding (const std::string& newName)
    {
        m_prev_names.emplace_back(name);
        name = newName;
        
        if(reg) {
            reg->addInternal(newName, reg->FindByID(m_ID));
        }
    }
    
private:
    inline CBase::ptr clone(CRegister * reg) const override {
        return std::make_shared<CWoman>(m_ID, GetName(), m_prev_names, reg);
    }

    inline std::string intermezzo() const override
    {
        std::stringstream ss;
        if (!m_prev_names.empty()) {
            ss << " [";
            for (std::size_t id = 0; id < m_prev_names.size(); ++id) {
                if (id) { ss << ", "; }
                ss << m_prev_names[id];
            }
            ss << "]";
        }
        
        return ss.str();
    }
    
    inline std::string type() const override { return "woman"; }
    
    std::vector<std::string> m_prev_names;
};


#ifndef __PROGTEST__
template <typename T_>
static bool        vectorMatch ( const vector<T_>     & res,
                                 const vector<string> & ref )
{
  vector<string> tmp;
  for ( const auto & x : res )
  {
    ostringstream oss;
    oss << *x;
    tmp . push_back ( oss . str () );
  }
    if (tmp != ref) {
        std::cout << "reference: \n";
        for (const auto& v : ref) {
            std::cout << v << std::endl;
        }
        std::cout << "\nstudent:\n";
        for (const auto& v : res) {
            std::cout << *v << std::endl;
        }
    }
  return tmp == ref;
}
template <typename T_>
static bool        listMatch ( const list<pair<T_, ERel> >     & res,
                               const list<pair<string, ERel> > & ref )
{
  list<pair<string, ERel> > tmp;
  for ( const auto & x : res )
  {
    ostringstream oss;
    oss << *x . first;
    tmp . emplace_back ( oss . str (), x . second );
  }
    if (tmp != ref) {
        std::cout << "reference: \n";
        for (const auto& v : ref) {
            std::cout << v.first << std::endl;
        }
        std::cout << "\nstudent:\n";
        for (const auto& v : res) {
            std::cout << *v.first << std::endl;
        }
    }
  return tmp == ref;
}
int main ( void )
{
   /* auto testMan = make_shared<CMan> ( 1, "Peterson George" );
    auto testW = make_shared<CWoman> ( 2, "Petersonova Georgina" );
    std::cout << *testMan << "\n" << *testW << std::endl;
    testW->Wedding("Pepegovic");
    testW->Wedding("Pacalova");
    std::cout << *testW << std::endl;*/
  ostringstream oss;
  CRegister r;
  assert ( r . Add ( make_shared<CMan> ( 1, "Peterson George" ),
                     nullptr, nullptr ) == true );
  assert ( r . Add ( make_shared<CMan> ( 2, "Watson Paul" ),
                     nullptr, nullptr ) == true );
  assert ( r . Add ( make_shared<CMan> ( 10, "Smith Samuel" ),
                     nullptr, nullptr ) == true );
  assert ( r . Add ( make_shared<CWoman> ( 11, "Peterson Jane" ),
                     r . FindByID ( 1 ), nullptr ) == true );
  assert ( r . Add ( make_shared<CWoman> ( 12, "Peterson Sue" ),
                     r . FindByID ( 1 ), nullptr ) == true );
  assert ( r . Add ( make_shared<CMan> ( 13, "Pershing John" ),
                     nullptr, nullptr ) == true );
  assert ( r . Add ( make_shared<CMan> ( 14, "Pearce Joe" ),
                     nullptr, nullptr ) == true );
  assert ( r . Add ( make_shared<CMan> ( 15, "Peant Thomas" ),
                     nullptr, nullptr ) == true );
  assert ( r . Add ( make_shared<CMan> ( 100, "Smith John" ),
                     r . FindByID ( 10 ), r . FindByID ( 11 ) ) == true );
  assert ( r . Add ( make_shared<CMan> ( 101, "Smith Roger" ),
                     r . FindByID ( 10 ), r . FindByID ( 11 ) ) == true );
  assert ( r . Add ( make_shared<CMan> ( 102, "Smith Daniel" ),
                     r . FindByID ( 10 ), r . FindByID ( 11 ) ) == true );
  assert ( r . Add ( make_shared<CWoman> ( 103, "Smith Eve" ),
                     r . FindByID ( 10 ), r . FindByID ( 11 ) ) == true );
  assert ( r . Add ( make_shared<CWoman> ( 103, "Smith Jane" ),
                     r . FindByID ( 10 ), r . FindByID ( 11 ) ) == false );
  dynamic_cast<CWoman &> ( *r . FindByID ( 12 ) ) . Wedding ( "Smith Sue" );
  assert ( r . Add ( make_shared<CMan> ( 150, "Pershing Joe" ),
                     r . FindByID ( 13 ), r . FindByID ( 12 ) ) == true );
  dynamic_cast<CWoman &> ( *r . FindByID ( 12 ) ) . Wedding ( "Pearce Sue" );
  assert ( r . Add ( make_shared<CMan> ( 151, "Pearce Phillip" ),
                     r . FindByID ( 14 ), r . FindByID ( 12 ) ) == true );
  dynamic_cast<CWoman &> ( *r . FindByID ( 12 ) ) . Wedding ( "Peant Sue" );
  assert ( r . Add ( make_shared<CMan> ( 152, "Peant Harry" ),
                     r . FindByID ( 15 ), r . FindByID ( 12 ) ) == true );
  assert ( r . Add ( make_shared<CMan> ( 200, "Pershing Peter" ),
                     r . FindByID ( 150 ), r . FindByID ( 103 ) ) == true );
  assert ( r . Add ( make_shared<CWoman> ( 201, "Pershing Julia" ),
                     r . FindByID ( 150 ), r . FindByID ( 103 ) ) == true );
  assert ( r . Add ( make_shared<CWoman> ( 202, "Pershing Anne" ),
                     r . FindByID ( 150 ), r . FindByID ( 103 ) ) == true );
  assert ( vectorMatch ( r . FindByName ( "Peterson" ), vector<string>
           {
             "1: Peterson George (man)",
             "11: Peterson Jane (woman)",
             "12: Peant Sue [Peterson Sue, Smith Sue, Pearce Sue] (woman)"
           } ) );
  assert ( vectorMatch ( r . FindByName ( "Pe" ), vector<string>
           {
             "1: Peterson George (man)",
             "11: Peterson Jane (woman)",
             "12: Peant Sue [Peterson Sue, Smith Sue, Pearce Sue] (woman)",
             "13: Pershing John (man)",
             "14: Pearce Joe (man)",
             "15: Peant Thomas (man)",
             "150: Pershing Joe (man)",
             "151: Pearce Phillip (man)",
             "152: Peant Harry (man)",
             "200: Pershing Peter (man)",
             "201: Pershing Julia (woman)",
             "202: Pershing Anne (woman)"
           } ) );
  assert ( vectorMatch ( r . FindByName ( "Smith" ), vector<string>
           {
             "10: Smith Samuel (man)",
             "12: Peant Sue [Peterson Sue, Smith Sue, Pearce Sue] (woman)",
             "100: Smith John (man)",
             "101: Smith Roger (man)",
             "102: Smith Daniel (man)",
             "103: Smith Eve (woman)"
           } ) );
  assert ( r . FindByID ( 1 ) -> GetID () == 1 );
  oss . str ( "" );
  oss << * r . FindByID ( 1 );
  assert ( oss . str () == "1: Peterson George (man)" );
  oss . str ( "" );
  r . FindByID ( 1 ) -> PrintDescendants ( oss );
    std::cout << oss.str() << std::endl;
  assert ( oss . str () ==
    "1: Peterson George (man)\n"
    " +- 11: Peterson Jane (woman)\n"
    " |  +- 100: Smith John (man)\n"
    " |  +- 101: Smith Roger (man)\n"
    " |  +- 102: Smith Daniel (man)\n"
    " |  \\- 103: Smith Eve (woman)\n"
    " |     +- 200: Pershing Peter (man)\n"
    " |     +- 201: Pershing Julia (woman)\n"
    " |     \\- 202: Pershing Anne (woman)\n"
    " \\- 12: Peant Sue [Peterson Sue, Smith Sue, Pearce Sue] (woman)\n"
    "    +- 150: Pershing Joe (man)\n"
    "    |  +- 200: Pershing Peter (man)\n"
    "    |  +- 201: Pershing Julia (woman)\n"
    "    |  \\- 202: Pershing Anne (woman)\n"
    "    +- 151: Pearce Phillip (man)\n"
    "    \\- 152: Peant Harry (man)\n" );
    
    oss.str("");
    r . FindByID ( 100 ) -> PrintDescendants ( oss );
      std::cout << oss.str() << std::endl;
    oss.str("");
  assert ( r . FindByID ( 2 ) -> GetID () == 2 );
  oss . str ( "" );
  oss << * r . FindByID ( 2 );
  assert ( oss . str () == "2: Watson Paul (man)" );
  oss . str ( "" );
  r . FindByID ( 2 ) -> PrintDescendants ( oss );
  assert ( oss . str () ==
    "2: Watson Paul (man)\n" );
  assert ( listMatch ( r . FindRelatives ( 100, 1 ), list<pair<string, ERel> >
           {
             { "11: Peterson Jane (woman)", ERel::REL_MOTHER },
             { "1: Peterson George (man)", ERel::REL_FATHER }
           } ) );
/*  assert ( listMatch ( r . FindRelatives ( 100, 13 ), list<pair<string, ERel> >
           {
             { "10: Smith Samuel (man)", ERel::REL_FATHER },
             { "103: Smith Eve (woman)", ERel::REL_DAUGHTER },
             { "200: Pershing Peter (man)", ERel::REL_SON },
             { "150: Pershing Joe (man)", ERel::REL_FATHER },
             { "13: Pershing John (man)", ERel::REL_FATHER }
           } ) );*/
  assert ( listMatch ( r . FindRelatives ( 100, 2 ), list<pair<string, ERel> >
           {
           } ) );
  try
  {
    r . FindRelatives ( 100, 3 );
    assert ( "Missing an exception" == nullptr );
  }
  catch ( const invalid_argument & e )
  {
  }
  catch ( ... )
  {
    assert ( "An unexpected exception thrown" );
  }
  try
  {
    r . FindRelatives ( 100, 100 );
    assert ( "Missing an exception" == nullptr );
  }
  catch ( const invalid_argument & e )
  {
  }
  catch ( ... )
  {
    assert ( "An unexpected exception thrown" );
  }
  return 0;
}
#endif /* __PROGTEST__ */


