/*
 * This file tests the implementation of the Party class in party.cc. You
 * shouldn't need to modify this file (and we will test your code against
 * an unmodified version). Please report any bugs to the course staff.
 *
 * Note that passing these tests doesn't guarantee that your code is correct
 * or meets the specifications given, but hopefully it's at least pretty
 * close.
 */

#include <atomic>
#include <cstdarg>
#include <functional>
#include <thread>

#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "party.cc"

// Interval for nanosleep corresponding to 1 ms.
struct timespec one_ms = {.tv_sec = 0, .tv_nsec = 1000000};

// Total number of guests that have been matched in this experiment.
std::atomic<int> matched;

// Total number of guests that have started execution in this experiment
// (used to ensure a particular ordering of guests, but it's not perfect).
std::atomic<int> started;

// Total number of errors detected. //
int num_errors;

/**
 * Verify whether a guest matched as expected and print info about
 * a match or error.
 * \param guest
 *      Name of a particular guest.
 * \param expected
 *      Expected match for that guest (empty string means no match expected
 *      yet).
 * \param actual
 *      Actual match for the guest so far: zero means no one has been
 *      matched to this guest yet.
 * \return
 *      1 means that expected and actual didn't match, zero means they did.
 */
int check_match(std::string guest, std::string expected, std::string actual)
{
    if (actual == expected) {
        if (expected.length() > 0) {
            printf("%s received %s as its match\n",
                    guest.c_str(), actual.c_str());
        }
        return 0;
    } else if (expected.length() == 0) {
        printf("Error: %s matched prematurely with %s\n",
                guest.c_str(), actual.c_str());
        return 1;
    } else if (actual.length() == 0) {
        printf("Error: %s was supposed to receive %s as match, but it "
                "hasn't matched yet\n",
                guest.c_str(), expected.c_str());
        return 1;
    } else {
        printf("Error: %s was supposed to receive %s as match, but it "
                "received %s instead\n",
                guest.c_str(), expected.c_str(), actual.c_str());
        return 1;
    }
}

/*
 * Wait for a given number of matches to occur.
 * \param count
 *      Wait until matched reaches this value
 * \param ms
 *      Return after this many milliseconds even if match hasn't
 *      reached the desired value.
 * \return
 *      True means the function succeeded, false means it failed.
 */
bool wait_for_matches(int count, int ms)
{
    while (1) {
        if (matched >= count) {
            return true;
        }
        if (ms <= 0) {
            return false;
        }
        nanosleep(&one_ms, nullptr);
        ms -= 1;
    }
}

/**
 * Invokes the meet method on a Party, records result information.
 * \param party
 *      Party object to invoke.
 * \param name
 *      Name of entering guest.
 * \param my_sign
 *       Zodiac sign of entering guest.
 * \param other_sign
 *       Desired Zodiac sign for match.
 * \param other_name
 *       The name of the matching guest is stored here.
 */
void guest(Party *party, std::string name, int my_sign, int other_sign,
        std::string *other_name)
{
    started++;
    *other_name = party->meet(name, my_sign, other_sign);
    matched++;
}

// Each method below tests a particular scenario.

void two_guests_perfect_match(void)
{
    Party party1;
    std::string match_a, match_b;

    matched = 0;
    printf("guest_a arrives: sign 0, other_sign 5\n");
    std::thread guest_a(guest, &party1, "guest_a", 0, 5, &match_a);
    guest_a.detach();
    printf("guest_b arrives: sign 5, other_sign 0\n");
    std::thread guest_b(guest, &party1, "guest_b", 5, 0, &match_b);
    guest_b.detach();
    wait_for_matches(2, 100);
    check_match("guest_a", "guest_b", match_a);
    check_match("guest_b", "guest_a", match_b);
}

void return_in_order(void)
{
    Party party1;
    std::string match_a, match_b, match_c, match_d, match_e, match_f;

    started = 0;
    matched = 0;
    printf("guest_a arrives: sign 1, other_sign 3\n");
    std::thread guest_a(guest, &party1, "guest_a", 1, 3, &match_a);
    guest_a.detach();
    while (started < 1) /* Do nothing */;
    printf("guest_b arrives: sign 1, other_sign 3\n");
    std::thread guest_b(guest, &party1, "guest_b", 1, 3, &match_b);
    guest_b.detach();
    while (started < 2) /* Do nothing */;
    printf("guest_c arrives: sign 1, other_sign 3\n");
    std::thread guest_c(guest, &party1, "guest_c", 1, 3, &match_c);
    guest_c.detach();
    while (started < 3) /* Do nothing */;
    wait_for_matches(1, 10);
    if (check_match("guest_a", "", match_a) + check_match("guest_b", "", match_b)
            + check_match("guest_c", "", match_c)) {
        return;
    }

    printf("guest_d arrives: sign 3, other_sign 1\n");
    std::thread guest_d(guest, &party1, "guest_d", 3, 1, &match_d);
    guest_d.detach();
    wait_for_matches(2, 100);
    if (check_match("guest_a", "guest_d", match_a)
            + check_match("guest_b", "", match_b)
            + check_match("guest_c", "", match_c)
            + check_match("guest_d", "guest_a", match_d)) {
        return;
    }

    printf("guest_e arrives: sign 3, other_sign 1\n");
    std::thread guest_e(guest, &party1, "guest_e", 3, 1, &match_e);
    guest_e.detach();
    wait_for_matches(4, 100);
    if (check_match("guest_b", "guest_e", match_b)
            + check_match("guest_c", "", match_c)
            + check_match("guest_e", "guest_b", match_e)) {
        return;
    }

    printf("guest_f arrives: sign 3, other_sign 1\n");
    std::thread guest_f(guest, &party1, "guest_f", 3, 1, &match_f);
    guest_f.detach();
    wait_for_matches(6, 100);
    if (check_match("guest_c", "guest_f", match_c)
            + check_match("guest_f", "guest_c", match_f)) {
        return;
    }
}

void sign_matching(void)
{
    Party party1;
    std::string match_a, match_b, match_c, match_d, match_e, match_f;

    matched = 0;
    printf("guest_a arrives: sign 1, other_sign 3\n");
    std::thread guest_a(guest, &party1, "guest_a", 1, 3, &match_a);
    guest_a.detach();
    printf("guest_b arrives: sign 2 other_sign 1\n");
    std::thread guest_b(guest, &party1, "guest_b", 2, 1, &match_b);
    guest_b.detach();
    printf("guest_c arrives: sign 3, other_sign 2\n");
    std::thread guest_c(guest, &party1, "guest_c", 3, 2, &match_c);
    guest_c.detach();
    wait_for_matches(1, 10);
    if (check_match("guest_a", "", match_a) + check_match("guest_b", "", match_b)
            + check_match("guest_c", "", match_c)) {
        return;
    }

    printf("guest_d arrives: sign 3, other_sign 1\n");
    std::thread guest_d(guest, &party1, "guest_d", 3, 1, &match_d);
    guest_d.detach();
    wait_for_matches(2, 100);
    if (check_match("guest_a", "guest_d", match_a)
            + check_match("guest_d", "guest_a", match_d)) {
        return;
    }

    printf("guest_e arrives: sign 2, other_sign 3\n");
    std::thread guest_e(guest, &party1, "guest_e", 2, 3, &match_e);
    guest_e.detach();
    wait_for_matches(4, 100);
    if (check_match("guest_c", "guest_e", match_c)
            + check_match("guest_e", "guest_c", match_e)) {
        return;
    }

    printf("guest_f arrives: sign 1, other_sign 2\n");
    std::thread guest_f(guest, &party1, "guest_f", 1, 2, &match_f);
    guest_f.detach();
    wait_for_matches(6, 100);
    if (check_match("guest_b", "guest_f", match_b)
            + check_match("guest_f", "guest_b", match_f)) {
        return;
    }
}

void single_sign(void)
{
    Party party1;
    std::string match_a, match_b;

    matched = 0;
    printf("guest_a arrives: sign 2, other_sign 2\n");
    std::thread guest_a(guest, &party1, "guest_a", 2, 2, &match_a);
    guest_a.detach();
    printf("guest_b arrives: sign 2, other_sign 2\n");
    std::thread guest_b(guest, &party1, "guest_b", 2, 2, &match_b);
    guest_b.detach();
    wait_for_matches(2, 100);
    if (check_match("guest_a", "guest_b", match_a)
            + check_match("guest_b", "guest_a", match_b)) {
        return;
    }
}

int
main(int argc, char *argv[])
{
    if (argc == 1) {
        printf("Available tests are:\n  two_guests_perfect_match\n  "
                "return_in_order\n  sign_matching\n  single_sign\n");
    }
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "two_guests_perfect_match") == 0) {
            two_guests_perfect_match();
        } else if (strcmp(argv[i], "return_in_order") == 0) {
            return_in_order();
        } else if (strcmp(argv[i], "sign_matching") == 0) {
            sign_matching();
        } else if (strcmp(argv[i], "single_sign") == 0) {
            single_sign();
        } else {
            printf("No test named '%s'\n", argv[i]);
        }
    }
}