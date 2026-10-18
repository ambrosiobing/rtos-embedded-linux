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
