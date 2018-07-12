#ifndef __BLITZ_EXAMPLE_APP_H_
#define __BLITZ_EXAMPLE_APP_H_ 1
class App {
 public:
  static App&  GetInstance() {
    static App app;
    return app;
  }
  blitz::Framework& get_framework() { return framework_; }
  int Init();

 
 private:
  blitz::Framework framework_;
  
};
#endif // __BLITZ_EXAMPLE_APP_H_
