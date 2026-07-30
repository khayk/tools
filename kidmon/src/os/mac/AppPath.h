#pragma once

#include <filesystem>

namespace fs = std::filesystem;

namespace km {

/**
 * @brief Map an executable path back to the application it belongs to.
 *
 * proc_pidpath names the vnode a process exec'd from, and a vnode with several
 * hard links has several equally valid names -- which one comes back is up to
 * the kernel. Chrome exploits this: at launch it hard links its main executable
 * into a temporary "code sign clone" bundle, so that replacing
 * /Applications/Google Chrome.app during an in-place update cannot pull the
 * file out from under the running process and invalidate its code signature.
 * The moment that update lands, the /Applications name is gone and the running
 * process starts reporting the temporary one:
 *
 *   /private/var/folders/<..>/X/com.google.Chrome.code_sign_clone/
 *       code_sign_clone.WtNnhr/Google Chrome.app.bundle/Contents/MacOS/Google Chrome
 *
 * That path has a random per-launch component, and it disappears when the app
 * exits, so it is useless for identifying the application afterwards. This maps
 * it back to the installed bundle.
 *
 * Paths that are not code sign clones -- the overwhelming majority -- are
 * returned unchanged, as is a clone whose application cannot be located.
 *
 * @note An app that updated itself while running is genuinely executing the
 *       previous build; the returned path points at the installed one. That is
 *       the intent here (which application is in use), but it does mean the
 *       path no longer names the exact bytes in memory.
 */
fs::path resolveRealAppPath(const fs::path& execPath);

} // namespace km
