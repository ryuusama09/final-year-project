

#ifndef __SubmatrixQueries__rmq__
#define __SubmatrixQueries__rmq__

#include <iostream>
#include <vector>
#include <valarray>
#include <cassert>
#include <algorithm>


template <typename T>
class BasicRQNode {
    T _value;
    
    size_t _minIndex, _maxIndex;
    bool _isLeaf;
    const BasicRQNode<T> *_highIndicesNode;
    const BasicRQNode<T> *_lowIndicesNode;
    
    typedef const T& (*compareFunctionPtr)(T const&, T const&);
    compareFunctionPtr _compare;
    
public:
    
    inline T value() const { return _value; }
    inline size_t minIndex() const { return _minIndex; }
    inline size_t maxIndex() const { return _maxIndex; }
    inline bool isLeaf() const { return _isLeaf; }
    inline const BasicRQNode<T> *highIndicesNode() const { return _highIndicesNode; }
    inline const BasicRQNode<T> *lowIndicesNode() const { return _lowIndicesNode; }
    inline compareFunctionPtr compareFunction() const { return _compare; }
    
    BasicRQNode(const std::vector<T> *values, size_t minIndex, size_t maxIndex, const T& (*compareFunc)(T const&, T const&) ) :
        _minIndex(minIndex),_maxIndex(maxIndex),_compare(compareFunc)
    {
        assert(minIndex <= maxIndex);
        
        if (minIndex == maxIndex) {
            _isLeaf = true;
            _value = (*values)[minIndex];
        }else{
            _isLeaf = false;
            
            size_t midIndex = minIndex + (maxIndex - minIndex)/2;
            _lowIndicesNode  = new BasicRQNode<T>(values, minIndex, midIndex, compareFunc);
            _highIndicesNode = new BasicRQNode<T>(values, midIndex+1, maxIndex, compareFunc);
            
            _value = (*_compare)(_lowIndicesNode->value(),_highIndicesNode->value());
        }
    }
    
    BasicRQNode(const std::valarray<T> *values, size_t minIndex, size_t maxIndex, const T& (*compareFunc)(T const&, T const&) ) :
    _minIndex(minIndex),_maxIndex(maxIndex),_compare(compareFunc)
    {
        assert(minIndex <= maxIndex);
        
        if (minIndex == maxIndex) {
            _isLeaf = true;
            _value = (*values)[minIndex];
        }else{
            _isLeaf = false;
            
            size_t midIndex = minIndex + (maxIndex - minIndex)/2;
            _lowIndicesNode  = new BasicRQNode<T>(values, minIndex, midIndex, compareFunc);
            _highIndicesNode = new BasicRQNode<T>(values, midIndex+1, maxIndex, compareFunc);
            
            _value = (*_compare)(_lowIndicesNode->value(),_highIndicesNode->value());
        }
    }
    
    BasicRQNode(T val,const T& (*compareFunc)(T const&, T const&)) : _minIndex(0), _maxIndex(0), _isLeaf(true), _value(val),_compare(compareFunc)
    {}

// Remove that constructor because it creates some seg faults when deallocating
//    BasicRQNode(BasicRQNode<T> *nodePtr) : _minIndex(nodePtr->minIndex()), _maxIndex(nodePtr->maxIndex()), _isLeaf(nodePtr->isLeaf()), _value(nodePtr->value()), _compare(nodePtr->compareFunction())
//    {
//        if (!this->isLeaf()) {
//            _lowIndicesNode = new BasicRQNode<T>(nodePtr->lowIndicesNode());
//            _highIndicesNode = new BasicRQNode<T>(nodePtr->highIndicesNode());
//        }
//    }
    ~BasicRQNode()
    {
        if (!_isLeaf) {
            delete _highIndicesNode;
            delete _lowIndicesNode;
        }
    }
    
    T query(size_t startIndex, size_t endIndex) const
    {
        assert(startIndex <= endIndex);

        size_t maxIndex = this->maxIndex();
        size_t minIndex = this->minIndex();
        
        // make sure the ranges intersects 
        assert(startIndex >= minIndex);
        assert(endIndex <= maxIndex);

        if (minIndex >= endIndex && maxIndex <= startIndex) {
            return this->value();
        }

        size_t midIndex = minIndex + (maxIndex - minIndex)/2;
        
        if (startIndex > midIndex) {
            return this->highIndicesNode()->query(startIndex,endIndex);
        }else if(endIndex <= midIndex){
            return this->lowIndicesNode()->query(startIndex,endIndex);
        }
        return (*_compare)(this->lowIndicesNode()->query(startIndex,midIndex),this->highIndicesNode()->query(midIndex+1,endIndex));
    }
};

#endif /* defined(__SubmatrixQueries__rmq__) */
