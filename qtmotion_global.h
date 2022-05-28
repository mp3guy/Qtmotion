#pragma once

#if defined(QTMOTION_LIBRARY)
#define QTMOTION_EXPORT Q_DECL_EXPORT
#else
#define QTMOTION_EXPORT Q_DECL_IMPORT
#endif
