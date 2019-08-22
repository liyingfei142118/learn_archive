#include <iostream>
using namespace std;

class shape
{
  public:
	shape(int a, int b)
	{
		width = a;
		height = b;
	}
	void setWidth(int w)
	{
		width = w;
	}
	void setHeight(int h)
	{
		height = h;
	}
	int area()
	{
		cout<<"shape"<<endl;
		return 0;
	}
  protected:
	int width;
	int height;
};
class A:public shape
{
	public:
		A(int a =  0, int b = 0):shape(a, b) {}

		int area ()
		{
			cout<<"A"<<endl;
			return 0;
		}
};
class B:public shape
{
	public:
		B(int a, int b):shape(a, b) {}
	
		int area()
		{
			cout<<"B"<<endl;
			return 0;
		}
};
class painCost
{
	public:
		int getCost(int area)
		{
			return (area * 70);
		}
};

class rectangle: public shape, public painCost
{
  public:
	int getArea()
	{
		return (width*height);
	}
};

int main()
{
    rectangle rec;
    rec.setWidth(5);
    rec.setHeight(7);
    int area = rec.getArea();

    cout<<"total area:"<<rec.getArea()<<endl;
    cout<<"total paint cost:"<<rec.getCost(area)<<endl;

    std::cout<<"hello world!"<<std::endl;

    A a(1, 2);
    B b(3, 4);
    shape *sh;
   
    sh = &a;
    sh->area();

   sh = &b;
   sh->area();
    return 0;

}
