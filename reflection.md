# Reflection – RemoteOps Assignment (IT24101658)

**Word count:** ~420 words

---

### 1. Which AI tools, if any, did you use, and at which stages of the work?

I used AI tools (mainly Grok and occasionally ChatGPT) during the development of Part 1. I mainly used them in the early stages for understanding how to structure a multi-threaded TCP server in C and for clarifying the exact protocol format required by the assignment. I also used AI when I faced specific errors such as “Address already in use” and when I needed help understanding how to correctly handle partial `recv()` calls and exact byte counting for file transfer (PUT/GET). I did not use AI during the Lab Assessment or Viva preparation, as those are closed-book.

### 2. What did the AI do well? Where did it get things wrong or mislead you?

The AI was helpful in explaining concepts clearly, such as the difference between blocking and non-blocking sockets, and in suggesting a clean structure for handling multiple clients with pthreads. It also provided useful examples of how to implement line-based reading and exact byte transfer.

However, the AI sometimes generated code that did not fully match the assignment’s strict protocol (for example, missing the SID tag or using incorrect response formats). In some cases, it suggested adding extra features that were not required, or it produced code that compiled but failed during actual testing (especially with file transfer and UDP monitoring). I had to carefully test and correct these issues myself.

### 3. What did you change, add, or reject from any AI output, and why?

I rejected several parts of the AI-generated code. For example, I removed unnecessary complexity in the concurrency model and rewrote the command handling logic so that it strictly followed the given protocol. I also changed the way authentication and session state were managed to make it clearer and easier to explain. I added proper logging, graceful disconnect handling, and the personalised values (port 9410, SID:3333, token OPS-3333, storage path, etc.) myself. I rejected any suggestion to extend the EXEC whitelist or to use high-level libraries, because the assignment requires pure BSD sockets and a fixed whitelist.

### 4. What did you learn about your own understanding of network programming through completing this assignment?

This assignment significantly improved my understanding of network programming. I learned how important correct framing is when dealing with TCP (partial reads, multiple messages in one buffer, and exact byte counting for binary data). I also gained practical experience with multi-threading, session management, and the differences between TCP and UDP. Most importantly, I realised that simply getting code from AI is not enough — I must fully understand every part of it, because I will be examined on it in the Lab Assessment and Viva. Writing and debugging the code myself helped me build real confidence in working with sockets at a low level.
