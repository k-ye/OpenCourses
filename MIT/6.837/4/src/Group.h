#ifndef GROUP_H
#define GROUP_H


#include "Object3D.h"
#include "Ray.h"
#include "Hit.h"
#include <vector>

using  namespace std;

///TODO: 
///Implement Group
///Add data structure to store a list of Object* 
class Group:public Object3D
{
public:

  Group()
  : _objs(0, NULL) {

  }

  ~Group(){
   
  }

  bool intersect(const Ray &r, float tmin, Hit &h) const override {
    bool hasHit = false;
    for (size_t i = 0; i < _objs.size(); ++i)
      hasHit |= _objs[i]->intersect(r, tmin, h);
    return hasHit;
  }
    
  void addObject(Object3D* obj)
  {
    _objs.push_back(obj);
  }

  int getGroupSize(){ 
    return _objs.size();
  }

 private:
  std::vector<Object3D*> _objs;
};

#endif
    
