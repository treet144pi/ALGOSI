#include <iostream>
#include <vector>
#include <memory>
enum class FigureType {
    RECTANGLE,
    SQUARE,
    SEGMENT
};

class Figure {
public:
    virtual std::string Name() const = 0;
    virtual int Perimeter() const = 0;
    virtual int Area() const = 0;
    virtual ~Figure() = default;
};

class Square: public Figure {
    private:
        int side_;

    public:
        explicit Square(int a): side_(a) {}

        std::string Name() const override  {
            return "Square";
        }
        int Perimeter() const override {
            return side_*4;
        }
        int Area() const override {
            return side_*side_;
        }

};

class Rectangle: public Figure {
    private:
        int widht_;
        int height_;
    public:
        Rectangle(int a, int b): widht_(a), height_(b) {}

        std::string Name() const override  {
            return "Segment";
        }
        int Perimeter() const override {
            return 2*(widht_+height_);
        }
        int Area() const override {
            return widht_*height_;
        }
};

class Segment: public Figure {
    private:
        int length_;

    public:
        Segment(int a): length_(a) {}

        std::string Name() const override  {
            return "Rectangle";
        }
        int Perimeter() const override {
            return length_;
        }
        int Area() const override {
            return 0;
        }


};

using FigurePtr = std::unique_ptr<Figure>;

// Фабричная функция
FigurePtr CreateFigure(FigureType type, int a, int b = 0) {
    if (type == FigureType::SQUARE) {
        return std::make_unique<Square>(a);
    }
    else if (type == FigureType::RECTANGLE) {
        return std::make_unique<Rectangle>(a,b);
    }
    else if (type == FigureType::SEGMENT) {
        return std::make_unique<Segment>(a);
    }
    else
    {
        return nullptr;
    }

}

// стратегия
class PrintStrategy {
public:
    virtual std::string Print(const Figure& f) const = 0;
    virtual ~PrintStrategy() = default;
};

class ShortPrintStrategy : public PrintStrategy {
public:
    std::string Print(const Figure& f) const override {
        return f.Name();
    }
};

class FullPrintStrategy : public PrintStrategy {
public:
    std::string Print(const Figure& f) const override {
        return f.Name() + " P=" + std::to_string(f.Perimeter()) +
               " S=" + std::to_string(f.Area());
    }
};

int main() {

    std::vector<FigurePtr> figures(3);
    figures[0] = CreateFigure(FigureType::RECTANGLE, 2, 3);
    figures[1] = CreateFigure(FigureType::SQUARE, 5);
    figures[2] = CreateFigure(FigureType::SEGMENT, 7);

    ShortPrintStrategy short_printer;
    FullPrintStrategy full_printer;

    for (const auto& f : figures) {
        std::cout << short_printer.Print(*f) << std::endl;
        std::cout << full_printer.Print(*f) << std::endl;
    }

    return 0;
}
