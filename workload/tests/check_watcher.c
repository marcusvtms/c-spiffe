/**
 *
 * (C) Copyright 2020-2021 Hewlett Packard Enterprise Development LP
 *
 *
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may
 * not use this file except in compliance with the License. You may obtain
 * a copy of the License at
 *
 *
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 *
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
 * License for the specific language governing permissions and limitations
 * under the License.
 *
 */

#include "c-spiffe/workload/watcher.h"
#include <check.h>

// callback that sets an int to a value, and ignores the context.
void set_int_callback(workloadapi_X509Context *context, void *_args)
{
    void **args = (void **) _args;
    long int val = (long int) arrpop(args);
    long int *var = (long int *) arrpop(args);
    *var = val;
}

// callback that increments an int by a value, and ignores the context.
void inc_int_callback(workloadapi_X509Context *context, void *_args)
{
    void **args = (void **) _args;
    long int val = (long int) arrpop(args);
    long int *var = (long int *) arrpop(args);
    *var = *(var) + val;
}

START_TEST(test_workloadapi_Watcher_callback_is_called_on_update_once)
{
    // variable to check callback
    long int toModify = 0;
    void **args = NULL;

    arrpush(args, (void *) &toModify);
    arrpush(args, (void *) 10);

    // callback object
    workloadapi_X509Callback callback;
    callback.func = set_int_callback;
    callback.args = (void *) args;

    // add callback to watcher.
    workloadapi_Watcher *watcher
        = (workloadapi_Watcher *) calloc(1, sizeof *watcher);
    watcher->x509callback = callback;

    // call update -> toModify = 10
    workloadapi_Watcher_OnX509ContextUpdate(watcher, NULL);
    arrfree(args);
    ck_assert_int_eq(toModify, 10);

    arrpush(args, (void *) &toModify);
    arrpush(args, (void *) 2);
    callback.func = inc_int_callback;
    callback.args = (void *) args;
    watcher->x509callback = callback;

    // call update -> toModify += 2
    workloadapi_Watcher_OnX509ContextUpdate(watcher, NULL);
    ck_assert_int_eq(toModify, 12);
    arrfree(args);

    arrpush(args, (void *) &toModify);
    arrpush(args, (void *) 5);
    callback.func = inc_int_callback;
    callback.args = (void *) args;
    watcher->x509callback = callback;

    // call update -> toModify += 5
    workloadapi_Watcher_OnX509ContextUpdate(watcher, NULL);
    ck_assert_int_eq(toModify, 17);
    arrfree(args);
    free(watcher);
}
END_TEST

void empty_callback(workloadapi_X509Context *context, void *_args) { return; }

START_TEST(test_workloadapi_newWatcher_creates_client_if_null)
{
    // empty but valid callback object
    workloadapi_X509Callback callback;
    callback.func = empty_callback;
    callback.args = NULL;

    // empty but valid watcher config
    workloadapi_WatcherConfig config;
    config.client = NULL; // client == NULL means create a new one.
    config.client_options = NULL;

    // error not set.
    err_t error = NO_ERROR;

    // create watcher with null client.
    workloadapi_Watcher *watcher
        = workloadapi_newWatcher(config, callback, &error);

    // new watcher succeded
    ck_assert_ptr_ne(watcher, NULL);
    // a new client was created and watcher owns it.
    ck_assert_ptr_ne(watcher->client, NULL);
    ck_assert_int_eq(watcher->owns_client, true);

    // There was no error.
    ck_assert_uint_eq(error, NO_ERROR);

    // free allocated watcher.
    workloadapi_Watcher_Free(watcher);
}
END_TEST

START_TEST(test_workloadapi_newWatcher_uses_provided_client)
{
    // empty but valid callback object
    workloadapi_X509Callback callback;
    callback.func = empty_callback;
    callback.args = NULL;

    // empty but valid watcher config
    workloadapi_WatcherConfig config;
    config.client = (void *) 1; // non-null already exists;
    config.client_options = NULL;

    // error not set.
    err_t error = NO_ERROR;

    // create watcher with null client.
    workloadapi_Watcher *watcher
        = workloadapi_newWatcher(config, callback, &error);

    // new watcher succeded
    ck_assert_ptr_ne(watcher, NULL);

    // uses client provided, and doesn't own client.
    ck_assert_ptr_eq(watcher->client, (void *) 1);
    ck_assert_int_eq(watcher->owns_client, false);

    // There was no error.
    ck_assert_uint_eq(error, NO_ERROR);

    // free allocated watcher.
    error = workloadapi_Watcher_Free(watcher);
    ck_assert_int_eq(error, NO_ERROR);
}
END_TEST

void setAddress(workloadapi_Client *client, void *not_used)
{
    workloadapi_Client_SetAddress(client, "http://example.com");
}
void setHeader(workloadapi_Client *client, void *not_used)
{
    workloadapi_Client_SetHeader(client, "workload.example.io", "true");
}

START_TEST(test_workloadapi_newWatcher_applies_Options)
{
    // empty but valid callback object
    workloadapi_X509Callback callback;
    callback.func = empty_callback;
    callback.args = NULL;

    // empty but valid watcher config
    workloadapi_WatcherConfig config;
    config.client_options = NULL;

    // error not set.
    err_t error = NO_ERROR;

    config.client = workloadapi_NewClient(&error);
    arrput(config.client_options, setAddress);
    arrput(config.client_options, setHeader);

    // create watcher with null client.
    workloadapi_Watcher *watcher
        = workloadapi_newWatcher(config, callback, &error);

    // new watcher succeded
    ck_assert_ptr_ne(watcher, NULL);
    // a new client was created and watcher owns it.
    ck_assert_ptr_eq(watcher->client, config.client);
    ck_assert_int_eq(watcher->owns_client, false);

    ck_assert_ptr_ne(config.client->address, NULL);
    ck_assert_uint_eq(strlen(config.client->address),
                      strlen("http://example.com"));
    ck_assert_uint_eq(strcmp(config.client->address, "http://example.com"), 0);

    ck_assert_ptr_ne(config.client->headers, NULL);
    ck_assert_uint_eq(strlen(config.client->headers[0]),
                      strlen("workload.example.io"));
    ck_assert_uint_eq(strlen(config.client->headers[1]), strlen("true"));
    ck_assert_uint_eq(strcmp(config.client->headers[0], "workload.example.io"),
                      0);
    ck_assert_uint_eq(strcmp(config.client->headers[1], "true"), 0);

    // There was no error.
    ck_assert_uint_eq(error, NO_ERROR);

    // free allocated watcher.
    workloadapi_Client_Free(config.client);
    workloadapi_Watcher_Free(watcher);
}
END_TEST

int waitAndUpdate(void *args)
{
    struct timespec now = { 3, 0 };
    thrd_sleep(&now, NULL);
    workloadapi_Watcher_TriggerUpdated((workloadapi_Watcher *) args);
    return 0;
}

START_TEST(test_workloadapi_Watcher_TimedWaitUntilUpdated_blocks);
{
    // empty but valid callback object
    workloadapi_X509Callback callback;
    callback.func = empty_callback;
    callback.args = NULL;

    // empty but valid watcher config
    workloadapi_WatcherConfig config;
    config.client_options = NULL;

    // error not set.
    err_t error = NO_ERROR;

    config.client = workloadapi_NewClient(&error);
    arrput(config.client_options, setAddress);
    arrput(config.client_options, setHeader);

    workloadapi_Watcher *watcher
        = workloadapi_newWatcher(config, callback, &error);

    struct timespec then;
    timespec_get(&then, TIME_UTC);
    thrd_t thread;
    thrd_create(&thread, waitAndUpdate, watcher);
    struct timespec timeout = then;
    timeout.tv_sec += 5;
    workloadapi_Watcher_TimedWaitUntilUpdated(watcher, &timeout);

    struct timespec now;
    timespec_get(&now, TIME_UTC);

    ck_assert_int_ge(now.tv_sec, then.tv_sec + 2);
    ck_assert_int_lt(now.tv_sec, then.tv_sec + 5);

    // free allocated watcher.
    workloadapi_Client_Free(config.client);
    workloadapi_Watcher_Free(watcher);
}
END_TEST

START_TEST(test_workloadapi_Watcher_WaitUntilUpdated_blocks);
{
    // empty but valid callback object
    workloadapi_X509Callback callback;
    callback.func = empty_callback;
    callback.args = NULL;

    // empty but valid watcher config
    workloadapi_WatcherConfig config;
    config.client_options = NULL;

    // error not set.
    err_t error = NO_ERROR;

    config.client = workloadapi_NewClient(&error);
    arrput(config.client_options, setAddress);
    arrput(config.client_options, setHeader);

    workloadapi_Watcher *watcher
        = workloadapi_newWatcher(config, callback, &error);

    struct timespec then;
    timespec_get(&then, TIME_UTC);
    thrd_t thread;
    thrd_create(&thread, waitAndUpdate, watcher);

    workloadapi_Watcher_WaitUntilUpdated(watcher);

    struct timespec now;
    timespec_get(&now, TIME_UTC);

    ck_assert_int_ge(now.tv_sec, then.tv_sec + 2);
    ck_assert_int_lt(now.tv_sec, then.tv_sec + 5);

    // free allocated watcher.
    workloadapi_Client_Free(config.client);
    workloadapi_Watcher_Free(watcher);
}
END_TEST

START_TEST(test_workloadapi_Watcher_Start_blocks);
{
    // empty but valid callback object
    workloadapi_X509Callback callback;
    callback.func = empty_callback;
    callback.args = NULL;

    // empty but valid watcher config
    workloadapi_WatcherConfig config;
    config.client_options = NULL;

    // error not set.
    err_t error = NO_ERROR;

    config.client = NULL; // no client = create client
    arrput(config.client_options, setAddress);
    arrput(config.client_options, setHeader);

    workloadapi_Watcher *watcher
        = workloadapi_newWatcher(config, callback, &error);

    struct timespec then;
    timespec_get(&then, TIME_UTC);
    thrd_t thread;
    thrd_create(&thread, waitAndUpdate, watcher);
    error = workloadapi_Watcher_Start(watcher);

    ck_assert(!watcher->closed);
    struct timespec now;
    timespec_get(&now, TIME_UTC);
    
    ck_assert_int_ge(now.tv_sec, then.tv_sec + 2);
    ck_assert_int_lt(now.tv_sec, then.tv_sec + 5);

    // close watcher
    error = workloadapi_Watcher_Close(watcher);
    // free allocated watcher.

    workloadapi_Watcher_Free(watcher);
}
END_TEST

START_TEST(test_workloadapi_Watcher_Close);
{
    // empty but valid callback object
    workloadapi_X509Callback callback;
    callback.func = empty_callback;
    callback.args = NULL;

    // empty but valid watcher config
    workloadapi_WatcherConfig config;
    config.client_options = NULL;
    config.client = NULL; // no client = create client

    arrput(config.client_options, setAddress);

    // error not set.
    err_t error = NO_ERROR;

    workloadapi_Watcher *watcher
        = workloadapi_newWatcher(config, callback, &error);

    ck_assert(watcher->closed);
    ck_assert_ptr_ne(watcher->client, NULL);
    ck_assert(watcher->client->closed);
    ck_assert(!watcher->client->owns_stub);
    ck_assert_ptr_eq(watcher->client->stub, NULL);

    struct timespec then;
    timespec_get(&then, TIME_UTC);
    thrd_t thread;
    thrd_create(&thread, waitAndUpdate, watcher); // unblocks thread

    error = workloadapi_Watcher_Start(watcher);
    ck_assert(!watcher->closed);

    struct timespec now;
    timespec_get(&now, TIME_UTC);

    ck_assert_int_ge(now.tv_sec, then.tv_sec + 2);
    ck_assert_int_lt(now.tv_sec, then.tv_sec + 5);

    error = workloadapi_Watcher_Close(watcher);

    ck_assert(watcher->closed);
    ck_assert(watcher->close_error == NO_ERROR);
    ck_assert_ptr_ne(watcher->client, NULL);
    // free allocated watcher.
    workloadapi_Watcher_Free(watcher);
}
END_TEST

Suite *watcher_suite(void)
{
    Suite *s = suite_create("watcher");
    TCase *tc_core = tcase_create("core");
    tcase_add_test(tc_core,
                   test_workloadapi_Watcher_callback_is_called_on_update_once);
    tcase_add_test(tc_core,
                   test_workloadapi_newWatcher_creates_client_if_null);
    tcase_add_test(tc_core, test_workloadapi_newWatcher_uses_provided_client);
    tcase_add_test(tc_core, test_workloadapi_newWatcher_applies_Options);
    tcase_add_test(tc_core,
                   test_workloadapi_Watcher_TimedWaitUntilUpdated_blocks);
    tcase_add_test(tc_core, test_workloadapi_Watcher_WaitUntilUpdated_blocks);
    tcase_add_test(tc_core, test_workloadapi_Watcher_Start_blocks);
    tcase_add_test(tc_core, test_workloadapi_Watcher_Close);

    suite_add_tcase(s, tc_core);

    return s;
}

int main(int argc, char **argv)
{
    Suite *s = watcher_suite();
    SRunner *sr = srunner_create(s);
    srunner_run_all(sr, CK_NORMAL);
    const int number_failed = srunner_ntests_failed(sr);

    srunner_free(sr);

    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
