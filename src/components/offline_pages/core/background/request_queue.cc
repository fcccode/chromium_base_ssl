// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/offline_pages/core/background/request_queue.h"

#include <utility>

#include "base/bind.h"
#include "base/location.h"
#include "base/threading/thread_task_runner_handle.h"
#include "components/offline_pages/core/background/add_request_task.h"
#include "components/offline_pages/core/background/change_requests_state_task.h"
#include "components/offline_pages/core/background/get_requests_task.h"
#include "components/offline_pages/core/background/initialize_store_task.h"
#include "components/offline_pages/core/background/mark_attempt_aborted_task.h"
#include "components/offline_pages/core/background/mark_attempt_completed_task.h"
#include "components/offline_pages/core/background/mark_attempt_deferred_task.h"
#include "components/offline_pages/core/background/mark_attempt_started_task.h"
#include "components/offline_pages/core/background/pick_request_task.h"
#include "components/offline_pages/core/background/reconcile_task.h"
#include "components/offline_pages/core/background/remove_requests_task.h"
#include "components/offline_pages/core/background/request_queue_store.h"
#include "components/offline_pages/core/background/save_page_request.h"

namespace offline_pages {

namespace {
// Completes the get requests call.
void GetRequestsDone(RequestQueue::GetRequestsCallback callback,
                     bool success,
                     std::vector<std::unique_ptr<SavePageRequest>> requests) {
  GetRequestsResult result =
      success ? GetRequestsResult::SUCCESS : GetRequestsResult::STORE_FAILURE;
  std::move(callback).Run(result, std::move(requests));
}

// Completes the add request call.
void AddRequestDone(RequestQueue::AddRequestCallback callback,
                    const SavePageRequest& request,
                    ItemActionStatus status) {
  AddRequestResult result;
  switch (status) {
    case ItemActionStatus::SUCCESS:
      result = AddRequestResult::SUCCESS;
      break;
    case ItemActionStatus::ALREADY_EXISTS:
      result = AddRequestResult::ALREADY_EXISTS;
      break;
    case ItemActionStatus::STORE_ERROR:
      result = AddRequestResult::STORE_FAILURE;
      break;
    case ItemActionStatus::NOT_FOUND:
    default:
      NOTREACHED();
      return;
  }
  std::move(callback).Run(result, request);
}

}  // namespace

RequestQueue::RequestQueue(std::unique_ptr<RequestQueueStore> store)
    : store_(std::move(store)), task_queue_(this), weak_ptr_factory_(this) {
  Initialize();
}

RequestQueue::~RequestQueue() {}

void RequestQueue::OnTaskQueueIsIdle() {}

void RequestQueue::GetRequests(GetRequestsCallback callback) {
  std::unique_ptr<Task> task(new GetRequestsTask(
      store_.get(), base::BindOnce(&GetRequestsDone, std::move(callback))));
  task_queue_.AddTask(std::move(task));
}

void RequestQueue::AddRequest(const SavePageRequest& request,
                              AddRequestCallback callback) {
  std::unique_ptr<AddRequestTask> task(new AddRequestTask(
      store_.get(), request,
      base::BindOnce(&AddRequestDone, std::move(callback), request)));
  task_queue_.AddTask(std::move(task));
}

void RequestQueue::RemoveRequests(const std::vector<int64_t>& request_ids,
                                  UpdateCallback callback) {
  std::unique_ptr<Task> task(
      new RemoveRequestsTask(store_.get(), request_ids, std::move(callback)));
  task_queue_.AddTask(std::move(task));
}

void RequestQueue::ChangeRequestsState(
    const std::vector<int64_t>& request_ids,
    const SavePageRequest::RequestState new_state,
    UpdateCallback callback) {
  std::unique_ptr<Task> task(new ChangeRequestsStateTask(
      store_.get(), request_ids, new_state, std::move(callback)));
  task_queue_.AddTask(std::move(task));
}

void RequestQueue::MarkAttemptStarted(int64_t request_id,
                                      UpdateCallback callback) {
  std::unique_ptr<Task> task(new MarkAttemptStartedTask(
      store_.get(), request_id, std::move(callback)));
  task_queue_.AddTask(std::move(task));
}

void RequestQueue::MarkAttemptAborted(int64_t request_id,
                                      UpdateCallback callback) {
  std::unique_ptr<Task> task(new MarkAttemptAbortedTask(
      store_.get(), request_id, std::move(callback)));
  task_queue_.AddTask(std::move(task));
}

void RequestQueue::MarkAttemptCompleted(int64_t request_id,
                                        FailState fail_state,
                                        UpdateCallback callback) {
  std::unique_ptr<Task> task(new MarkAttemptCompletedTask(
      store_.get(), request_id, fail_state, std::move(callback)));
  task_queue_.AddTask(std::move(task));
}

void RequestQueue::MarkAttemptDeferred(int64_t request_id,
                                       UpdateCallback callback) {
  std::unique_ptr<Task> task(new MarkAttemptDeferredTask(
      store_.get(), request_id, std::move(callback)));
  task_queue_.AddTask(std::move(task));
}

void RequestQueue::PickNextRequest(
    OfflinerPolicy* policy,
    ClientPolicyController* policy_controller,
    PickRequestTask::RequestPickedCallback picked_callback,
    PickRequestTask::RequestNotPickedCallback not_picked_callback,
    PickRequestTask::RequestCountCallback request_count_callback,
    DeviceConditions conditions,
    const std::set<int64_t>& disabled_requests,
    base::circular_deque<int64_t>* prioritized_requests) {
  // Using the PickerContext, create a picker task.
  std::unique_ptr<Task> task(new PickRequestTask(
      store_.get(), policy, policy_controller, std::move(picked_callback),
      std::move(not_picked_callback), std::move(request_count_callback),
      std::move(conditions), disabled_requests, prioritized_requests));

  // Queue up the picking task, it will call one of the callbacks when it
  // completes.
  task_queue_.AddTask(std::move(task));
}

void RequestQueue::ReconcileRequests(UpdateCallback callback) {
  std::unique_ptr<Task> task(
      new ReconcileTask(store_.get(), std::move(callback)));

  // Queue up the reconcile task.
  task_queue_.AddTask(std::move(task));
}

void RequestQueue::CleanupRequestQueue() {
  // Create a cleanup task.
  std::unique_ptr<Task> task(cleanup_factory_->CreateCleanupTask(store_.get()));

  // Queue up the cleanup task.
  task_queue_.AddTask(std::move(task));
}

void RequestQueue::Initialize() {
  std::unique_ptr<Task> task(new InitializeStoreTask(
      store_.get(), base::BindOnce(&RequestQueue::InitializeStoreDone,
                                   weak_ptr_factory_.GetWeakPtr())));
  task_queue_.AddTask(std::move(task));
}

void RequestQueue::InitializeStoreDone(bool success) {
  // TODO(fgorski): Result can be ignored for now. Report UMA in future.
  // No need to pass the result up to RequestCoordinator.
}

}  // namespace offline_pages