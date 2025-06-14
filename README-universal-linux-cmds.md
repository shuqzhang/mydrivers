1. User management
1.1 id [username] -- query username's info

1.2 groups [username] -- query username's group

1.3 awk -F: '{ print $1 }' /etc/passwd  -- query all users

1.4 cat /etc/groups  -- query all groups




2. Process management
2.1 ps -ef  --  display all running processes

2.2 cat /proc/[process_num]/exe  -- display process execute program, owner



3. Memory management



4. Network management