# IOCP Server Engine

C++23으로 구현한 Windows 기반 비동기 게임 서버 프로젝트입니다.
IOCP와 멀티스레드를 기반으로 네트워크 처리, 세션 관리, 패킷 처리 및 서버의 주요 기반 구조를 직접 구현하고 검증했습니다.

## Tech Stack

* C++23
* Windows Socket / IOCP
* Multithreading
* CMake / GNU Make
* SQLite

## 주요 구현

### Network

* IOCP 기반 Overlapped I/O
* `AcceptEx`를 이용한 비동기 연결 수락
* `DisconnectEx`를 이용한 비동기 연결 종료
* 패킷 송수신 및 직렬화 / 역직렬화
* Broadcast 및 Login 처리
* Fragmentation / Sticky Packet 대응

### Session Management

* Session Pool 구현
* `SharedPoolPtr` 기반 참조 카운팅 및 객체 생명주기 관리
* 세션 상태별 관리를 위한 State-based Sparse Set 구현
* 멀티스레드 환경에서 세션 상태 변경 동기화

### Task Scheduler

* Central Queue
* Local Queue
* Work Stealing
* Batch Processing

여러 스케줄링 방식을 직접 구현하고 동일한 조건에서 성능을 측정하여 비교했습니다.

## 테스트 및 검증

네트워크 계층의 안정성을 확인하기 위해 다음 테스트를 수행했습니다.

| Test                    | Description               |
| ----------------------- | ------------------------- |
| Fragmentation           | 분할된 패킷의 정상적인 조립 검증        |
| Sticky Packets          | 병합된 패킷의 정상적인 분리 검증        |
| Connection Stress       | 대량 재접속 환경에서 세션 및 자원 관리 검증 |
| Broadcast               | 다중 클라이언트에 대한 패킷 전파 검증     |
| Invalid Packet Handling | 비정상 패킷 처리 검증              |
| Login                   | 인증 및 세션 관리 검증             |

### Connection Stress

* 100 Threads
* 총 100,000회 연결 / 해제
* 성공 100,000회 / 실패 0회
* 약 2.96초 소요

## Troubleshooting

개발 과정에서 발생한 주요 문제와 해결 과정을 별도로 기록했습니다.

* [Major Troubleshooting](./Troubleshooting.md)

## Build

```bash
make all
```

주요 명령어:

```bash
make server
make run-server
make tests
make run-tests
make clean
```

## Environment

* OS: Windows
* Compiler: MSVC
* C++ Standard: C++23
* Build Tool: GNU Make 4.4.1
* Optimization: `-O3`
