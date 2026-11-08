/* projects/05-the-zones/src/build_stamp.h: what the image is, declared where anything can print it.
 *
 * THE DEFINITIONS ARE NOT IN THE TREE. They are generated into the build directory by
 * scripts/stamp.py, which the project's CMakeLists.txt runs at every build, so the strings
 * describe the link that produced the image rather than the last compile of whichever file
 * happened to contain a `__DATE__`.
 *
 * WHY THAT DISTINCTION IS NOT PEDANTRY. The banner's old form was `__DATE__` and `__TIME__` in
 * main.c, added so that a stale flash could be told from a reproduction. On Saturday 17 October
 * 2026 at 16:51 an image built after an edit to the driver printed a stamp 69 minutes old,
 * because main.c had not been recompiled and did not need to be. The image was new and the
 * stamp said it was not, so the guard against reading stale output became a reason to discard a
 * real result. scripts/stamp.py carries the full reasoning.
 *
 * AND "AT EVERY BUILD" WAS NOT TRUE UNTIL FRIDAY 6 NOVEMBER 2026, which is recorded here
 * because this file is where a reader comes to find out what the stamp guarantees. The rule
 * depended on a custom target carrying no output, which is unconditional under make and already
 * satisfied under ninja, so the generated file was written once when a build directory was
 * created and never again. On Monday 26 October 2026 four bench captures printed a stamp eleven
 * hours old from images that were all correct, and the first reading of them was that the flash
 * was stale. **The same failure as the one above, one layer down**, and in the same direction:
 * what a false stale report throws away is a real result.
 *
 * The fix is the SYMBOLIC property in the project's CMakeLists.txt, which is measured rather
 * than reasoned, because the obvious fix was also wrong. scripts/test_build_stamp.py builds four
 * wirings twice each and keeps two of them failing on purpose, so the guarantee this header
 * describes is checked instead of asserted.
 */
#ifndef P05_BUILD_STAMP_H
#define P05_BUILD_STAMP_H

/* When this image was linked, written in full with the weekday, computed rather than taken from
 * the machine's locale so two laptops produce comparable logs. */
extern const char build_stamp_when[];

/* The commit it was built from, and how many files in this project were edited or untracked at
 * that moment. A bench build usually has some, and a stamp that hid them would be claiming the
 * image matches a published history when it does not. */
extern const char build_stamp_commit[];

#endif /* P05_BUILD_STAMP_H */
