/*
 * Binary Manager - Executable Build Management
 * 
 * A focused tool for managing and rebuilding project executables.
 * 
 * Features:
 * - Displays all executable binaries in build/
 * - Shows binary details (size, last modified, make target)
 * - Individual rebuild buttons for each binary
 * - Real-time binary scanning
 * 
 * Usage: ./binary_manager [port]
 * Default port: 9006
 * 
 * Access at: http://localhost:9006
 */

#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <cstring>
#include <ctime>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <thread>

struct Binary {
    std::string name;
    std::string path;
    std::string target_name;
    std::string type; // demo, test, tool
    size_t file_size;
    time_t last_modified;
    std::string make_command;
    bool is_executable;
};

std::vector<Binary> scan_binaries() {
    std::vector<Binary> binaries;
    
    // Find executable files in build directory
    std::string cmd = "find /Users/mehranghamaty/wkspace/ToolBox/build -type f -perm +111 ! -name '*.dylib' ! -name '*.so' ! -name '*.a' 2>/dev/null | grep -E '(demos|test)/'";
    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) return binaries;
    
    char buffer[1024];
    while (fgets(buffer, sizeof(buffer), pipe)) {
        std::string bin_path(buffer);
        bin_path.erase(bin_path.find_last_not_of(" \n\r\t") + 1);
        
        Binary bin;
        bin.path = bin_path;
        bin.is_executable = true;
        
        size_t last_slash = bin_path.rfind('/');
        bin.name = (last_slash != std::string::npos) ? bin_path.substr(last_slash + 1) : bin_path;
        
        // Determine type
        if (bin_path.find("/demos/") != std::string::npos) {
            bin.type = "demo";
        } else if (bin_path.find("/test/") != std::string::npos) {
            bin.type = "test";
        } else {
            bin.type = "tool";
        }
        
        struct stat st;
        if (stat(bin_path.c_str(), &st) == 0) {
            bin.file_size = st.st_size;
            bin.last_modified = st.st_mtime;
        }
        
        // Extract target name from path
        size_t demos_pos = bin_path.find("/demos/");
        size_t test_pos = bin_path.find("/test/");
        
        if (demos_pos != std::string::npos) {
            std::string relative = bin_path.substr(demos_pos + 7);
            size_t slash = relative.find('/');
            bin.target_name = (slash != std::string::npos) ? relative.substr(0, slash) : bin.name;
        } else if (test_pos != std::string::npos) {
            std::string relative = bin_path.substr(test_pos + 6);
            size_t slash = relative.find('/');
            bin.target_name = (slash != std::string::npos) ? relative.substr(0, slash) : bin.name;
        } else {
            bin.target_name = bin.name;
        }
        
        bin.make_command = "cmake --build build --target " + bin.target_name + " -j8";
        binaries.push_back(bin);
    }
    pclose(pipe);
    
    return binaries;
}

bool rebuild_binary(const std::string& target) {
    std::string cmd = "cd /Users/mehranghamaty/wkspace/ToolBox && cmake --build build --target " + target + " -j8 2>&1";
    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) return false;
    
    char buffer[256];
    bool success = true;
    while (fgets(buffer, sizeof(buffer), pipe)) {
        std::string line(buffer);
        if (line.find("error:") != std::string::npos) success = false;
    }
    
    int exit_code = pclose(pipe);
    return (exit_code == 0 && success);
}

std::string build_html() {
    std::string html = "<!DOCTYPE html><html><head><meta charset='UTF-8'>";
    html += "<title>Binary Manager</title><style>";
    html += "*{margin:0;padding:0;box-sizing:border-box}";
    html += "body{font-family:-apple-system,sans-serif;background:linear-gradient(135deg,#1a0a0a,#2e1a1a);color:#e0e0e0;padding:20px;min-height:100vh}";
    html += ".container{max-width:1200px;margin:0 auto}";
    html += "header{background:linear-gradient(135deg,#dc2626,#f97316);padding:30px;border-radius:12px;margin-bottom:30px;box-shadow:0 8px 32px rgba(220,38,38,0.3)}";
    html += "h1{font-size:32px;font-weight:700;margin-bottom:10px}";
    html += ".subtitle{font-size:14px;opacity:0.9}";
    html += ".card{background:rgba(255,255,255,0.05);border-radius:12px;padding:25px;margin-bottom:20px;backdrop-filter:blur(10px);border:1px solid rgba(255,255,255,0.1)}";
    html += ".filters{display:flex;gap:10px;margin-bottom:15px}";
    html += ".filter-btn{padding:8px 16px;border:none;border-radius:6px;font-size:12px;font-weight:600;cursor:pointer;background:rgba(255,255,255,0.1);color:#e0e0e0}";
    html += ".filter-btn.active{background:linear-gradient(135deg,#dc2626,#f97316);color:white}";
    html += "table{width:100%;border-collapse:collapse;margin-top:15px}";
    html += "th,td{text-align:left;padding:12px;border-bottom:1px solid rgba(255,255,255,0.1)}";
    html += "th{background:rgba(220,38,38,0.2);font-weight:600;color:#fca5a5}";
    html += ".btn{padding:8px 16px;border:none;border-radius:6px;font-size:12px;font-weight:600;cursor:pointer;transition:all 0.2s;margin-right:5px}";
    html += ".btn-primary{background:linear-gradient(135deg,#dc2626,#f97316);color:white}";
    html += ".btn-primary:hover{transform:translateY(-1px);opacity:0.9}";
    html += ".btn-secondary{background:rgba(255,255,255,0.1);color:#e0e0e0}";
    html += ".btn-secondary:hover{background:rgba(255,255,255,0.15)}";
    html += ".btn-run{background:linear-gradient(135deg,#16a34a,#22c55e);color:white}";
    html += ".btn-run:hover{transform:translateY(-1px);opacity:0.9}";
    html += ".empty{text-align:center;padding:40px;color:#9ca3af}";
    html += ".badge{display:inline-block;padding:4px 10px;border-radius:4px;font-size:11px;font-weight:600}";
    html += ".badge-demo{background:rgba(59,130,246,0.2);color:#93c5fd}";
    html += ".badge-test{background:rgba(168,85,247,0.2);color:#c4b5fd}";
    html += ".badge-tool{background:rgba(234,179,8,0.2);color:#fde047}";
    html += "</style></head><body><div class='container'>";
    html += "<header><h1>Binary Manager</h1><p class='subtitle'>Executable Build Management</p></header>";
    html += "<div class='card'><div style='display:flex;justify-content:space-between;align-items:center;margin-bottom:20px'>";
    html += "<h2 style='font-size:20px'>Executables <span id='count' style='color:#9ca3af;font-size:14px'>(0)</span></h2>";
    html += "<button class='btn btn-secondary' onclick='loadBinaries()'>Refresh</button></div>";
    html += "<div class='filters'>";
    html += "<button class='filter-btn active' onclick='filterType(\"all\")'>All</button>";
    html += "<button class='filter-btn' onclick='filterType(\"demo\")'>Demos</button>";
    html += "<button class='filter-btn' onclick='filterType(\"test\")'>Tests</button>";
    html += "<button class='filter-btn' onclick='filterType(\"tool\")'>Tools</button>";
    html += "</div>";
    html += "<table><thead><tr><th>Binary Name</th><th>Type</th><th>Size</th><th>Modified</th><th>Actions</th></tr></thead>";
    html += "<tbody id='binaries'><tr><td colspan='5' class='empty'>Loading...</td></tr></tbody></table></div></div>";
    html += "<script>";
    html += "let allBins=[];let currentFilter='all';";
    html += "async function loadBinaries(){try{const r=await fetch('/api/binaries');const d=await r.json();allBins=d.binaries;updateTable()}catch(e){console.error(e)}}";
    html += "function filterType(type){currentFilter=type;document.querySelectorAll('.filter-btn').forEach(b=>b.classList.remove('active'));";
    html += "event.target.classList.add('active');updateTable()}";
    html += "function updateTable(){const bins=currentFilter==='all'?allBins:allBins.filter(b=>b.type===currentFilter);";
    html += "const t=document.getElementById('binaries');const c=document.getElementById('count');";
    html += "c.textContent='('+bins.length+')';if(!bins.length){t.innerHTML='<tr><td colspan=5 class=empty>No binaries found</td></tr>';return}";
    html += "t.innerHTML=bins.map(bin=>{";
    html += "let s=bin.size<1024*1024?(bin.size/1024).toFixed(1)+' KB':(bin.size/(1024*1024)).toFixed(2)+' MB';";
    html += "let d=new Date(bin.last_modified*1000).toLocaleString('en-US',{month:'short',day:'numeric',hour:'2-digit',minute:'2-digit'});";
    html += "let badgeClass=bin.type==='demo'?'badge-demo':bin.type==='test'?'badge-test':'badge-tool';";
    html += "return '<tr><td style=\"font-family:monospace;color:#f87171\">'+bin.name+'</td><td><span class=\"badge '+badgeClass+'\">'+bin.type+'</span></td>'";
    html += "+'<td style=\"color:#9ca3af\">'+s+'</td><td style=\"color:#9ca3af\">'+d+'</td>'";
    html += "+'<td><button class=\"btn btn-primary\" onclick=\"rebuild(\\''+bin.target+'\\',\\''+bin.name+'\\')\">[!] Rebuild</button>'";
    html += "+'<button class=\"btn btn-run\" onclick=\"runBinary(\\''+bin.path+'\\',\\''+bin.name+'\\')\">[>] Run</button>'";
    html += "+'<button class=\"btn btn-secondary\" onclick=\"showCmd(\\''+bin.make_command+'\\')\">[?] Cmd</button></td></tr>'";
    html += "}).join('')}";
    html += "async function rebuild(t,n){if(!confirm('Rebuild '+n+'?'))return;try{";
    html += "const r=await fetch('/api/rebuild',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({target:t})});";
    html += "const d=await r.json();alert(d.success?'Rebuilt!':'Build failed');if(d.success)loadBinaries()}catch(e){alert('Error: '+e.message)}}";
    html += "function runBinary(path,name){alert('To run '+name+':\\n\\n'+path+'\\n\\nRun it from your terminal!')}";
    html += "function showCmd(c){alert('Build Command:\\n\\n'+c)}";
    html += "loadBinaries();setInterval(loadBinaries,10000);";
    html += "</script></body></html>";
    return html;
}

void handle_request(int client_fd) {
    char buffer[4096];
    ssize_t bytes_read = read(client_fd, buffer, sizeof(buffer) - 1);
    if (bytes_read <= 0) {
        close(client_fd);
        return;
    }
    buffer[bytes_read] = '\0';
    
    std::string request(buffer);
    std::string response;
    
    if (request.find("GET / ") == 0 || request.find("GET /index") == 0) {
        response = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n" + build_html();
    } else if (request.find("GET /api/binaries") == 0) {
        auto bins = scan_binaries();
        std::ostringstream json;
        json << "{\"binaries\":[";
        for (size_t i = 0; i < bins.size(); i++) {
            if (i > 0) json << ",";
            json << "{"
                 << "\"name\":\"" << bins[i].name << "\","
                 << "\"path\":\"" << bins[i].path << "\","
                 << "\"target\":\"" << bins[i].target_name << "\","
                 << "\"type\":\"" << bins[i].type << "\","
                 << "\"size\":" << bins[i].file_size << ","
                 << "\"last_modified\":" << bins[i].last_modified << ","
                 << "\"make_command\":\"" << bins[i].make_command << "\""
                 << "}";
        }
        json << "]}";
        response = "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\n\r\n" + json.str();
    } else if (request.find("POST /api/rebuild") == 0) {
        size_t body_pos = request.find("\r\n\r\n");
        if (body_pos != std::string::npos) {
            std::string body = request.substr(body_pos + 4);
            size_t target_pos = body.find("\"target\":\"");
            if (target_pos != std::string::npos) {
                size_t start = target_pos + 10;
                size_t end = body.find("\"", start);
                std::string target = body.substr(start, end - start);
                bool success = rebuild_binary(target);
                response = "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\n\r\n{\"success\":" + 
                          std::string(success ? "true" : "false") + "}";
            }
        }
    } else {
        response = "HTTP/1.1 404 Not Found\r\n\r\nNot Found";
    }
    
    write(client_fd, response.c_str(), response.length());
    close(client_fd);
}

int main(int argc, char* argv[]) {
    int port = 9006;
    if (argc > 1) {
        port = std::atoi(argv[1]);
        if (port < 1024 || port > 65535) port = 9006;
    }
    
    std::cout << "\n";
    std::cout << "================================================================\n";
    std::cout << "  Binary Manager - Executable Build Management\n";
    std::cout << "================================================================\n";
    std::cout << "  URL:  http://localhost:" << port << "\n";
    std::cout << "  Port: " << port << "\n";
    std::cout << "================================================================\n\n";
    
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        std::cerr << "Failed to create socket\n";
        return 1;
    }
    
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);
    
    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        std::cerr << "Port " << port << " already in use\n";
        close(server_fd);
        return 1;
    }
    
    if (listen(server_fd, 10) < 0) {
        std::cerr << "Listen failed\n";
        close(server_fd);
        return 1;
    }
    
    std::cout << "Server running\n";
    std::cout << "Scanning executables in build/demos and build/test\n\n";
    
    while (true) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
        
        if (client_fd >= 0) {
            std::thread(handle_request, client_fd).detach();
        }
    }
    
    close(server_fd);
    return 0;
}
