# Design of Lab 4's Login System

## Mechanisms for Securing

I used the SHA256 function to hash the user's password when creating account. 
I stored the hash in a file along with the rest of the user's info including uid & username. 
When a user logs in, their password is hashed and compared with the hashed value corresponding to that username.
###

To prevent against brute force attacks in which a malicious actor could construct rainbow tables, i.e., pre-compute a series of hashes for common passwords (e.g. 123456, "", password), I implemented salting. 
To supplement a pseudo-random seed, I used the syscall `uptime()` since there are no other hardware mechanisms for RNG. 
I then passed that through SHA256 and appended that to the password before hashing that concatenated string. 
I also stored the salt publicly with the uid, username, and hashed password so that it can be compared when a user logs in.

## Files Created

I store all the users (uid, username, password, salt) in one file (`/auth.txt`) sequentially.