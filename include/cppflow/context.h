//
// Created by serizba on 27/6/20.
//

#ifndef CPPFLOW2_CONTEXT_H
#define CPPFLOW2_CONTEXT_H

#include <memory>
#include <utility>
#include <mutex>

#include <tensorflow/c/c_api.h>
#include <tensorflow/c/eager/c_api.h>

namespace cppflow {

    bool status_check(TF_Status* status) {
        if (TF_GetCode(status) != TF_OK) {
            throw std::runtime_error(TF_Message(status));
        }
        return true;
    }

    class context {
        public:
            static TFE_Context* get_context();
            static TF_Status* get_status();

        private:
            TFE_Context* tfe_context{nullptr};
            static void init_global_context();

        public:
            explicit context(TFE_ContextOptions* opts);
            explicit context();

            context(context const&) = delete;
            context& operator=(context const&) = delete;
            context(context&&) noexcept;
            context& operator=(context&&) noexcept;

            ~context();
    };

    // TODO: create ContextManager class if needed
    inline context global_context;
    static std::once_flag flag1;

}

namespace cppflow {

    inline TFE_Context* context::get_context() {
        std::call_once(flag1, []() {context::init_global_context(); });
        return global_context.tfe_context;
    }

    inline TF_Status* context::get_status() {
        thread_local std::unique_ptr<TF_Status, decltype(&TF_DeleteStatus)> local_tf_status(TF_NewStatus(), &TF_DeleteStatus);
        return local_tf_status.get();
    }

    inline context::context(TFE_ContextOptions* opts) {
        auto tf_status = context::get_status();
        if(opts == nullptr) {
            std::unique_ptr<TFE_ContextOptions, decltype(&TFE_DeleteContextOptions)> opts(TFE_NewContextOptions(), &TFE_DeleteContextOptions);
            this->tfe_context = TFE_NewContext(opts.get(), tf_status);
        } else {
            this->tfe_context = TFE_NewContext(opts, tf_status);
        }
        status_check(tf_status);
    }

    inline context::context() = default;

   inline  void context::init_global_context()
    {
        if (global_context.tfe_context == nullptr)
        {
            auto tf_status = context::get_status();
            std::unique_ptr<TFE_ContextOptions, decltype(&TFE_DeleteContextOptions)> opts(
                TFE_NewContextOptions(), &TFE_DeleteContextOptions);
            global_context.tfe_context = TFE_NewContext(opts.get(), tf_status);
        }
    }
	
    inline context::context(context&& ctx) noexcept :
        tfe_context(std::exchange(ctx.tfe_context, nullptr))
    {
    }

    inline context& context::operator=(context&& ctx) noexcept {
        tfe_context = std::exchange(ctx.tfe_context, nullptr);
        return *this;
    }

    inline context::~context() {
        TFE_DeleteContext(this->tfe_context);
    }

}

#endif //CPPFLOW2_CONTEXT_H
