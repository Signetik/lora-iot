#ifndef __VERSION__H
#define __VERSION__H

extern const char* GIT_TAG;
extern const char* GIT_REV;
extern const char* GIT_BRANCH;

#define VERSION_MAJOR (GIT_TAG[1] - '0' + 0)
#define VERSION_MINOR (GIT_TAG[3] - '0' + 0)
#define VERSION_INCREMENTAL (GIT_TAG[5] - '0' + 0)
#define VERSION_ENGG (0)

#endif /* __VERSION__H */
