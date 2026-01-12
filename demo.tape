Output demo.gif

Set FontSize 16
Set Width 1200
Set Height 600
Set Padding 20
Set FontFamily "Courier New"

# Setup
Hide
Type "cc -o slit slit.c"
Enter
Type "clear"
Enter
Show

# Scenario 1: The Pipeline (Killer Feature)
Type "echo 'Error: Connection failed at 10.0.0.1' | ./slit | grep 'Fixed'"
Enter
Sleep 1s
Type "Fixed: Connection established at 10.0.0.1"
Sleep 500ms
Ctrl+A
Sleep 500ms
Type "Log - "
Sleep 500ms
Enter
Sleep 2s

# Clear for next scene
Type "clear"
Enter

# Scenario 2: File Editing & Features
Type "cat > config.json <<EOF"
Enter
Type "{"
Enter
Type '  "host": "localhost",'
Enter
Type '  "port": 8080,'
Enter
Type '  "debug": false,'
Enter
Type '  "comment": "TODO: Update"'
Enter
Type "}"
Enter
Type "EOF"
Enter
Type "clear"
Enter

Type "./slit +4 config.json"
Sleep 500ms
Enter
Sleep 1s

# Edit "false" to "true" using Ctrl+W
Right 11
Sleep 200ms
Ctrl+W
Sleep 500ms
Type "true"
Sleep 500ms
Enter
Sleep 1s

# Edit comment with Japanese (UTF-8 check)
Type "./slit 5 config.json"
Enter
Sleep 1s
Right 14
Sleep 200ms
Ctrl+K
Sleep 500ms
Type "設定を更新しました (Updated)"
Sleep 1s
Enter
Sleep 2s

# Show result
Type "cat config.json"
Enter
Sleep 3s
