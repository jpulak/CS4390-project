# CS4390-project

## Project: Peer-to-Peer File Sharing Protocol

### Overview
This project implements a **peer-to-peer file sharing protocol** with a **tracker server** that keeps track of shared files and peers. Each **peer** can upload, download, and share files with other peers. Peers communicate with the tracker server to create and update tracker files and request file transfers.

---

### Project Requirements
The project must include:

1. **Tracker Server**
   - Multi-threaded server that handles peer connections.
   - Commands supported: `LIST`, `GET`, `createtracker`, `updatetracker`.
   - Stores tracker files with peer and file info.
   - Removes inactive peers based on timestamps.

2. **Peer Program**
   - Multiple threads for uploading and downloading file segments.
   - Downloads files from multiple peers simultaneously.
   - Updates tracker server after downloading a complete segment.
   - Resumes incomplete downloads and deletes tracker files after completion.
   - Sends periodic `updatetracker` messages.

3. **Tracker File**
   - Contains metadata and list of peers sharing the file.
   - Format:
     ```
     Filename: <file-name>
     Filesize: <size in bytes>
     Description: <short description>
     MD5: <md5 checksum>
     <ip>:<port>:<start byte>:<end byte>:<timestamp>
     ...
     ```

4. **Protocol**
   - `createtracker` – create a tracker file on the server.
   - `updatetracker` – update peer info or file segments.
   - `LIST` – request a list of shared files.
   - `GET` – request a specific tracker file.

5. **Configuration Files**
   - **clientThreadConfig.cfg**: Tracker server IP, port, update interval.
   - **serverThreadConfig.cfg**: Peer listening port, shared folder name.

---

### Deliverables
- Source code for **tracker server** and **peer program**.
- **Makefile** for compiling programs.
- **Configuration files** for peers and server.
- **Final report (PDF)** with:
  - Team member names and roles
  - Installation and running guide
  - Code design explanation
  - References for any external libraries or code used

---

### Running the Project
1. **Tracker Server**
   ```bash
   make -f makefile
   ./tracker
````

2. **Peer Program**

   ```bash
   make -f makefile
   ./peer
   ```

* Peers should simulate multiple clients in separate folders (peer1, peer2, peer3).
* Commands can be sent manually for testing: `LIST`, `GET filename.track`, `createtracker`, `updatetracker`.

---

### Notes

* Each file should be divided into segments for download.
* Maximum segment size: 1024 bytes.
* Program must display necessary events on the screen (e.g., file downloads, tracker updates).
* Peers must handle incomplete files and resume downloads automatically.

```

