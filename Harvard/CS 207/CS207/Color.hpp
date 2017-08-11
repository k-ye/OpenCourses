#ifndef CS207_COLOR_HPP
#define CS207_COLOR_HPP

namespace CS207 {

/** @file Color.hpp
 * @brief Define the Color class for RGB color values. */

/** @class Color
 * @brief Class representing RGB color values. */
struct Color {
  typedef float value_type;

  value_type r, g, b;

  /** Construct the color black. */
  Color()
      : r(0), g(0), b(0) {
  }
  /** Construct a shade of grey.
   * @param[in] v Value (0 = black, 1 = white)
   * @pre 0 <= @a v <= 1 */
  Color(float v)
      : r(v), g(v), b(v) {
    assert(0 <= v && v <= 1);
  }
  /** Construct a color from red, green, and blue values.
   * @pre 0 <= @a r, @a g, @a b <= 1 */
  Color(float r, float g, float b)
    : r(r), g(g), b(b) {
    assert(0 <= r && r <= 1);
    assert(0 <= g && g <= 1);
    assert(0 <= b && b <= 1);
  }

  /** Construct a color from red, green, and blue values.
   * @pre 0 <= @a r, @a g, @a b <= 1 */
  static Color make_rgb(float r, float g, float b) {
    return Color(r, g, b);
  }

  /** Construct a color from hue, saturation, and value.
   * @param[in] h hue (0 = red, 0.166 = yellow, 0.333 = green, 0.5 = cyan,
   *                   0.666 = blue, 0.833 = magenta)
   * @param[in] s saturation (0 = white, 1 = saturated)
   * @param[in] v value (0 = dark,  1 = bright)
   * @pre 0 <= @a h, @a s, @a v <= 1 */
  static Color make_hsv(float h, float s, float v) {
    assert(0 <= h && h <= 1);
    assert(0 <= s && s <= 1);
    assert(0 <= v && v <= 1);

    if (s == 0)
      return Color(v);

    float var_h = (h == 1 ? 0 : 6*h);
    int var_i = int(var_h);         // Or var_i = floor(var_h)
    float var_1 = v * (1 - s);
    float var_2 = v * (1 - s * (var_h - var_i));
    float var_3 = v * (1 - s * (1 - (var_h - var_i)));
    switch (var_i) {
    case 0: // hue [0,1/6): red <-> yellow
      return Color(v, var_3, var_1);
    case 1: // hue [1/6,1/3): yellow <-> green
      return Color(var_2, v, var_1);
    case 2: // hue [1/3,1/2): green <-> cyan
      return Color(var_1, v, var_3);
    case 3: // hue [1/2,2/3): cyan <-> blue
      return Color(var_1, var_2, v);
    case 4: // hue [2/3,5/6): blue <-> magenta
      return Color(var_3, var_1, v);
    default: // hue [5/6,1): magenta <-> red
      return Color(v, var_1, var_2);
    }
  }

  /** Construct a color suitable for heat map display.
   * @param[in] v Heat value (0 = cold, 1 = hot)
   * @pre 0 <= @a v <= 1
   *
   * The resulting color is a fully-saturated bright color ranging from
   * purple/blue for cold to red for hot. */
  static Color make_heat(float v) {
    assert(0 <= v && v <= 1);
    return make_hsv(0.78f - 0.76f * v, 1, 1);
  }
}; // end Color

} // end namespace CS207
#endif
