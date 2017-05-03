# ERSS Final Project
## William Kim, Ja'lon Sisson

### Here, I will describe the structure of my code.
* First, we create the amazon_world socket and the amazon_ups socket and connect to them.
* Then, we connect to the local database we have.
* we have an optional step of initializing the database (adding some buy orders so that we can test without the front end), which can be turned off by setting "insertDB" at the top to 0
* Then, we fork into 2 processes. The child process handles sending new buy requests to the world and UPS. The parent process handles receiving responses from UPS and world.
* So the workflow is (connect to world and UPS) -> (fork into two processes) -> (child handles sending new buy requests to world and UPS)/(parent handles receiving responses).
* Please take a look at the source code to see the implementation of these (connection.cpp)