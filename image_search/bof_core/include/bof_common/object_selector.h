#ifndef IMAGE_SEARCH_FEATURE_ANALYZER_OBJECT_SELECTOR_H_
#define IMAGE_SEARCH_FEATURE_ANALYZER_OBJECT_SELECTOR_H_

#include <map>
#include <string>

template<class T> class ObjectSelector {
 private:
  typedef std::map<std::string, T*> ObjectsMap;

 public:
  ObjectSelector();
  ~ObjectSelector();

  static ObjectSelector* GetInstance();
  void Finalize();

  void AddObject(T *object);
  T* GenerateObject(const std::string &obj_name);
  void ReleaseObject(T *&obj);
  void ReleaseSelfObjects();

 private:
  ObjectsMap objects_;
  static ObjectSelector *instance_;
};

template<class T> ObjectSelector<T>::ObjectSelector() {
}

template <class T> ObjectSelector<T>::~ObjectSelector() {
  typename ObjectsMap::iterator it = objects_.begin();
  while (it != objects_.end()) {
    if (it->second) {
      delete it->second;
    }
    ++it;
  }
}

template <class T> ObjectSelector<T>* ObjectSelector<T>::GetInstance() {
  if (NULL == instance_) {
    instance_ = new ObjectSelector;
  }
  return instance_;
}

template<class T> void ObjectSelector<T>::AddObject(T *object) {
  typename ObjectsMap::iterator it = objects_.find(object->GetName());
  if (it == objects_.end()) {
    objects_.insert(std::pair<std::string, T*>(object->GetName(), object));
  }
}
  
template<class T> void ObjectSelector<T>::Finalize() {
  if (instance_) {
    delete instance_;
    instance_ = NULL;
  }
}

template<class T> T* ObjectSelector<T>::GenerateObject(const std::string &obj_name) {
  typename ObjectsMap::iterator it = objects_.find(obj_name);
  if (it != objects_.end()) {
    return it->second->Clone();
  }
  return NULL;
}

template<class T> void ObjectSelector<T>::ReleaseObject(T *&obj) {
  if (obj) {
    delete obj;
    obj = NULL;
  }
}

template<class T> void ObjectSelector<T>::ReleaseSelfObjects() {
  typename ObjectsMap::iterator it = objects_.begin();
  while (it != objects_.end()) {
    ReleaseObject(it->second);
    ++it;
  }
}


#define OBJECT_SELECTOR_ADD_OBJECT(TSelector, T) {      \
    T *obj = new T;                                     \
    TSelector::GetInstance()->AddObject(obj);           \
  }

void CreateObjectFactory();
void DestroyObjectFactory();

#endif
