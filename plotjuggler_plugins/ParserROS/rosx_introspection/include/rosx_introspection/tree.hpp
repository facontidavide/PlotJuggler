/*********************************************************************
* Software License Agreement (BSD License)
*
*  Copyright 2016-2017 Davide Faconti
*  All rights reserved.
*
*  Redistribution and use in source and binary forms, with or without
*  modification, are permitted provided that the following conditions
*  are met:
*
*   * Redistributions of source code must retain the above copyright
*     notice, this list of conditions and the following disclaimer.
*   * Redistributions in binary form must reproduce the above
*     copyright notice, this list of conditions and the following
*     disclaimer in the documentation and/or other materials provided
*     with the distribution.
*   * Neither the name of Willow Garage, Inc. nor the names of its
*     contributors may be used to endorse or promote products derived
*     from this software without specific prior written permission.
*
*  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
*  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
*  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
*  FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
*  COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
*  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
*  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
*  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
*  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
*  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
*  ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
*  POSSIBILITY OF SUCH DAMAGE.
* *******************************************************************/


#ifndef STRINGTREE_H
#define STRINGTREE_H

#include <vector>
#include <deque>
#include <iostream>
#include <memory>

#include "rosx_introspection/builtin_types.hpp"

namespace RosMsgParser {

namespace details{

/**
 * @brief Element of the tree. it has a single parent and N >= 0 children.
 */
template <typename T> class TreeNode
{

public:

  typedef std::vector<TreeNode> ChildrenVector; // dangerous because of pointer invalidation (but faster)

  TreeNode(const TreeNode* parent );

  const TreeNode* parent() const  { return _parent; }

  const T& value() const          { return _value; }
  void setValue( const T& value)  { _value = value; }

  const ChildrenVector& children()const   { return _children; }
  ChildrenVector& children()              { return _children; }

  const TreeNode* child(size_t index) const { return &(_children[index]); }
  TreeNode* child(size_t index) { return &(_children[index]); }

  TreeNode *addChild(const T& child );

  bool isLeaf() const { return _children.empty(); }

private:
  const TreeNode*   _parent = nullptr;
  T                 _value;
  ChildrenVector    _children;
};


template <typename T> class Tree
{
public:
  Tree(): _root( new TreeNode<T>(nullptr) ) {}

  /**
     * Find a set of elements in the tree and return the pointer to the leaf.
     * The first element of the concatenated_values should be a root of the Tree.
     * The leaf corresponds to the last element of concatenated_values in the Tree.
     */
  template<typename Vect> const TreeNode<T>* find( const Vect& concatenated_values, bool partial_allowed = false);

  /// Constant pointer to the root of the tree.
  const TreeNode<T>* croot() const { return _root.get(); }

  /// Mutable pointer to the root of the tree.
  TreeNode<T>* root() { return _root.get(); }


  friend std::ostream& operator<<(std::ostream& os, const Tree& _this){
    _this.print_impl(os, _this.croot() , 0);
    return os;
  }

  template <class Functor>
  void visit(Functor& func, const TreeNode<T>* node) const
  {
    func(node);
    for(auto child: node->children())
    {
      visit(func, &child);
    }
  }

private:

  void print_impl(std::ostream& os, const TreeNode<T> *node, int indent ) const;

  std::unique_ptr<TreeNode<T>> _root;
};

//-----------------------------------------


template <typename T> inline
std::ostream& operator<<(std::ostream &os, const std::pair<const TreeNode<T>*, const TreeNode<T>* >& tail_head )
{
  const TreeNode<T>* tail = tail_head.first;
  const TreeNode<T>* head = tail_head.second;

  if( !head ) return os;

  const TreeNode<T>* array[64];
  int index = 0;
  array[index++] = head;

  while( !head || head != tail)
  {
    head = head->parent();
    array[index++] = head;
  };
  array[index] = nullptr;
  index--;

  while ( index >=0)
  {
    if( array[index] ){
      os << array[index]->value();
    }
    if( index >0 )  os << ".";
    index--;
  }
  return os;
}

template <typename T> inline
void Tree<T>::print_impl(std::ostream &os, const TreeNode<T>* node, int indent) const
{
  for (int i=0; i<indent; i++) os << " ";
  os << node->value() << std::endl;

  for (const auto& child: node->children() )
  {
    print_impl(os, &child, indent+3);
  }
}

template <typename T> inline
TreeNode<T>::TreeNode(const TreeNode *parent):
  _parent(parent)
{

}

template <typename T> inline
TreeNode<T> *TreeNode<T>::addChild(const T& value)
{
  assert(_children.capacity() > _children.size() );
  _children.emplace_back( this );
  _children.back().setValue( value );
  return &_children.back();
}


template <typename T> template<typename Vect> inline
const TreeNode<T> *Tree<T>::find(const Vect& concatenated_values, bool partial_allowed )
{
  TreeNode<T>* node = &_root;

  for (const auto& value: concatenated_values)
  {
    bool found = false;
    for (auto& child: (node->children() ) )
    {
      if( child.value() == value)
      {
        node = &(child);
        found = true;
        break;
      }
    }
    if( !found ) return nullptr;
  }

  if( partial_allowed || node->children().empty() )
  {
    return  node;
  }
  return nullptr;
}

}

}



#endif // STRINGTREE_H
