#include <view/editor.h>
#include <view/menu.h>
#include <esp_log.h>

LIST_VIEW Editor_view{
    {"[ Editor ]"},
    {"- Edit Fidget Toy"},
};

template <typename T, size_t N>
constexpr size_t getArrayLength(T (&)[N])
{
  return N;
};

class C_EDITOR : public ListPage
{
protected:
public:
  void create()
  {
    this->setPageView("editor", Editor_view);
  }

  void router(uint8_t selectItmeNumber)
  {
    switch (selectItmeNumber)
    {
    case 0:
      gui->page_out_to(P_MENU);
      break;
      // case 1:
      //   gui->pageIn(M_EDIT_F0);
      //   break;
    }
  }
};

ListPage *P_EDITOR = new C_EDITOR;
