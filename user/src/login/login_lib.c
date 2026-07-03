#include <crypto.h>

#include "types.h"
#include "user.h"
#include "login.h"
#include "fcntl.h"

uchar *hash_salt(char *password, uchar *salt) {
    uint len = strlen(password);
    uchar *hash = malloc(SHA256_SIZE_BYTES);
    uchar salted_password[SHA256_SIZE_BYTES + len];
    for (int i = 0; i < len; i++)
        salted_password[i] = password[i];
    for (int i = 0; i < SHA256_SIZE_BYTES; i++)
        salted_password[i + len] = salt[i];
    sha256(salted_password, SHA256_SIZE_BYTES + len, hash);
    return hash;
}

void save_password(struct user *user, char *password) {
    int ticks = uptime();
    uchar salt[SHA256_SIZE_BYTES];
    sha256(&ticks, 4, salt);

    uchar *hash = hash_salt(password, salt);
    for (int i = 0; i < SHA256_SIZE_BYTES; i++) {
        user->salt[i] = salt[i];
        user->hash[i] = hash[i];
    }
    free(hash);
}

int str_cmp(const void *str1, const void *str2, int len) {
    const unsigned char *s1 = str1;
    const unsigned char *s2 = str2;
    for (int i = 0; i < len; i++) {
        if (s1[i] != s2[i]) return -1;
        if (s1[i] == '\0') break;
    }
    return 0;
}

int hash_cmp(struct user *user, char *password) {
    uchar *hash = hash_salt(password, user->salt);
    int diff = str_cmp(user->hash, hash, SHA256_SIZE_BYTES);
    free(hash);
    return diff;
}

/**
 * Hook into user/src/login/login_init.c in order to intialize any files or
 * data structures necessary for the login system
 * 
 * Called once per boot of xv6
 */
void init_hook() {
    int fd = open("auth.txt", O_CREATE | O_RDWR);
    if (fd == -1) exit();
    struct user user = {.uid = 0, .username = "root"};
    save_password(&user, "admin");
    write(fd, &user, sizeof(struct user));
    close(fd);
}

/**
 * Check if user exists in system 
 * 
 * @param username A null-terminated string representing the username
 * @return 0 on success if user exists, -1 for failure otherwise
 */
int does_user_exist(char *username) {
    int fd = open("auth.txt", O_RDWR);
    if (fd == -1) return -1;
    struct user user = {.uid = 0};
    while (read(fd, &user, sizeof(struct user)) && str_cmp(user.username, username, MAX_INPUT_SIZE));
    close(fd);
    return str_cmp(user.username, username, MAX_INPUT_SIZE);
}

/**
 * Create a user in the system associated with the username and password. Cannot
 * overwrite an existing username with a new password. Expectation is for
 * created users to have a unique non-root uid.
 * 
 * @param username A null-terminated string representing the username
 * @param password A null-terminated string representing the password
 * @return 0 on success, -1 for failure
 */
int create_user(char *username, char *password) {
    int fd = open("auth.txt", O_RDWR);
    if (fd == -1) return -1;
    struct user user = {.uid = 0};
    int i = 0;
    char c;
    while (read(fd, &user, sizeof(struct user))) {
        if (!str_cmp(user.username, username, MAX_INPUT_SIZE))
            return -1;
        user.uid++;
    }
    while (((c = username[i])) && i < MAX_INPUT_SIZE)
        user.username[i++] = c;
    while (i < MAX_INPUT_SIZE) {
        user.username[i++] = '\0';
    }
    save_password(&user, password);
    int result = write(fd, &user, sizeof(struct user)) ? 0 : -1;
    close(fd);
    return result;
}

void start_shell(int uid) {
    int pid, wpid;
    char *argv[] = {"sh", 0};
    for (;;) {
        printf(1, "init: starting sh\n");
        pid = fork();
        if (pid < 0) {
            printf(1, "init: fork failed\n");
            exit();
        }
        if (pid == 0) {
            setuid(uid);
            exec("sh", argv);
            printf(1, "init: exec sh failed\n");
            exit();
        }
        while ((wpid = wait()) >= 0 && wpid != pid)
            printf(1, "zombie!\n");
    }
}

/**
 * Login a user in the system associated with the username and password. Launch
 * the shell under the right permissions for the user. If no such user exists
 * or the password is incorrect, then login will fail.
 * 
 * @param username A null-terminated string representing the username
 * @param password A null-terminated string representing the password
 * @return no return on success, -1 for failure
 */
int login_user(char *username, char *password) {
    int fd = open("auth.txt", O_RDWR);
    if (fd == -1) return -1;
    struct user user = {.uid = 0};
    while (read(fd, &user, sizeof(struct user))) {
        if (!str_cmp(user.username, username, MAX_INPUT_SIZE)) {
            if (!hash_cmp(&user, password)) {
                close(fd);
                start_shell(user.uid);
            } else return -1;

        }
    }
    return -1;
}
