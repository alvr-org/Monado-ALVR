// Copyright 2024-2025, Collabora, Ltd.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief Content class function.
 * @author Simon Zeni <simon.zeni@collabora.com>
 * @ingroup aux_android
 */

#include "android_content.h"
#include "util/u_logging.h"

#include "wrap/android.content.h"
#include "wrap/java.io.h"

#include <string>

bool
android_content_get_files_dir(void *context, char *dir, size_t size)
{
	if (size == 0 || dir == NULL) {
		U_LOG_E("Invalid argument");
		return false;
	}

	wrap::java::io::File file = wrap::android::content::Context{(jobject)context}.getFilesDir();
	if (file.isNull()) {
		U_LOG_E("Failed to get File object");
		return false;
	}


	const std::string dirPath = file.getAbsolutePath();
	if (size < (dirPath.length() + 1)) {
		U_LOG_E("Output string is too small");
		return false;
	}

	dirPath.copy(dir, dirPath.length());
	dir[dirPath.length()] = '\0';
	return true;
}
