/*
 * Some simple vector maths
 */

struct vec2
{
    double x;
    double y;

    explicit vec2(double angle)
        : x(std::cos(angle)), y(std::sin(angle)) { }
    vec2(double x, double y)
        : x(x), y(y) { }
    vec2(vec2 const& other)
        : x(other.x), y(other.y) { }

    vec2& operator=(vec2 const& rhs) {
        x = rhs.x;
        y = rhs.y;
        return *this;
    }

    vec2 operator+(vec2 const& rhs) const {
        return vec2(x + rhs.x, y + rhs.y);
    }
    vec2 operator-(vec2 const& rhs) const {
        return vec2(x - rhs.x, y - rhs.y);
    }
    vec2 operator*(double r) const {
        return vec2(r*x, r*y);
    }
    vec2 operator/(double r) const {
        return vec2(x/r, y/r);
    }
    // Vector dot product
    double operator*(vec2 const& rhs) const {
        return x * rhs.x + y * rhs.y;
    }

    // Direction as an angle in radians
    double angle() const {
        return std::atan2(y, x);
    }

    // Vector's magnitude
    double length() const {
        return std::sqrt(x*x + y*y);
    }

    // Unit vector with same direction
    vec2 unit() const {
        return *this / length();
    }

    // Mutators (hissss)
    void length(double l) {
        *this = unit() * l;
    }

    void angle(double a) {
        *this = vec2(a) * length();
    }

    void operator+=(vec2 const& rhs) {
        x += rhs.x;
        y += rhs.y;
    }
    void operator-=(vec2 const& rhs) {
        x -= rhs.y;
        y -= rhs.y;
    }
    void operator*=(double r) {
        x *= r;
        y *= r;
    }
    void operator/=(double r) {
        x /= r;
        y /= r;
    }
};


vec2 operator*(double r, vec2 const& v) {
    return vec2(r*v.x, r*v.y);
}

