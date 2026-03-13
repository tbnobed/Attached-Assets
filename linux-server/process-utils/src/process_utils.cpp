#include <napi.h>
#include <unistd.h>
#include <fcntl.h>
#include <csignal>
#include <cstdlib>

static int g_devnull_fd = -1;
static int g_saved_stdout_fd = -1;
static int g_saved_stderr_fd = -1;

static void ensureDevNull() {
    if (g_devnull_fd < 0) {
        g_devnull_fd = open("/dev/null", O_WRONLY);
    }
}

Napi::Value RedirectStdoutToDevNull(const Napi::CallbackInfo& info) {
    ensureDevNull();
    if (g_saved_stdout_fd < 0) {
        g_saved_stdout_fd = dup(STDOUT_FILENO);
    }
    if (g_devnull_fd >= 0) {
        dup2(g_devnull_fd, STDOUT_FILENO);
    }
    return Napi::Number::New(info.Env(), g_saved_stdout_fd);
}

Napi::Value RedirectStderrToDevNull(const Napi::CallbackInfo& info) {
    ensureDevNull();
    if (g_saved_stderr_fd < 0) {
        g_saved_stderr_fd = dup(STDERR_FILENO);
    }
    if (g_devnull_fd >= 0) {
        dup2(g_devnull_fd, STDERR_FILENO);
    }
    return Napi::Number::New(info.Env(), g_saved_stderr_fd);
}

Napi::Value RestoreStdout(const Napi::CallbackInfo& info) {
    if (g_saved_stdout_fd >= 0) {
        dup2(g_saved_stdout_fd, STDOUT_FILENO);
    }
    return Napi::Boolean::New(info.Env(), true);
}

Napi::Value RestoreStderr(const Napi::CallbackInfo& info) {
    if (g_saved_stderr_fd >= 0) {
        dup2(g_saved_stderr_fd, STDERR_FILENO);
    }
    return Napi::Boolean::New(info.Env(), true);
}

static void sigintExitHandler(int sig) {
    (void)sig;
    _exit(0);
}

Napi::Value InstallSigintHandler(const Napi::CallbackInfo& info) {
    struct sigaction sa;
    sa.sa_handler = sigintExitHandler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESETHAND;
    sigaction(SIGINT, &sa, nullptr);
    return Napi::Boolean::New(info.Env(), true);
}

Napi::Object Init(Napi::Env env, Napi::Object exports) {
    exports.Set("redirectStdoutToDevNull", Napi::Function::New(env, RedirectStdoutToDevNull));
    exports.Set("redirectStderrToDevNull", Napi::Function::New(env, RedirectStderrToDevNull));
    exports.Set("restoreStdout", Napi::Function::New(env, RestoreStdout));
    exports.Set("restoreStderr", Napi::Function::New(env, RestoreStderr));
    exports.Set("installSigintHandler", Napi::Function::New(env, InstallSigintHandler));
    return exports;
}

NODE_API_MODULE(process_utils, Init)
